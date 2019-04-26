package token

import (
	"context"
	"strings"
	"time"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/metricsutil"
	"github.com/anki/sai-go-util/testutils/testtime"
	"github.com/anki/sai-service-framework/auth"
	"github.com/anki/sai-service-framework/grpc/grpcutil"
	"github.com/anki/sai-token-service/certstore"
	"github.com/anki/sai-token-service/client/clienthash"
	"github.com/anki/sai-token-service/db"
	"github.com/anki/sai-token-service/model"
	"github.com/anki/sai-token-service/proto/tokenpb"
	"github.com/anki/sai-token-service/stsgen"
	"github.com/anki/sai-token-service/tokenmetrics"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/satori/go.uuid"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

const (
	DefaultTokenExpiration  = 24 * time.Hour
	TokenRefreshWindowStart = 3 * time.Hour
	TokenRefreshWindowEnd   = 365 * 24 * time.Hour
)

type Server struct {
	Store       db.TokenStore
	CertStore   certstore.RobotCertificateStore
	StsGen      stsgen.Generator
	Signer      TokenSigner
	ClientStore ClientTokenStore
}

// ----------------------------------------------------------------------
// gRPC methods
// ----------------------------------------------------------------------

func (s *Server) AssociatePrimaryUser(ctx context.Context, req *tokenpb.AssociatePrimaryUserRequest) (*tokenpb.AssociatePrimaryUserResponse, error) {
	userId := auth.GetUserId(ctx)

	if userId == "" {
		alog.Warn{
			"action": "AssociatePrimaryUser",
			"status": "missing_user_id",
		}.LogC(ctx)
		return nil, status.Errorf(codes.Unauthenticated, "Missing user_id in accounts session")
	}

	requestorId := getCertificateName(ctx)
	token := makeToken(userId, requestorId)

	if len(req.SessionCertificate) > 0 {
		if parts := strings.Split(requestorId, ":"); len(parts) < 2 {
			alog.Warn{
				"action": "AssociatePrimaryUser",
				"status": "invalid_requestor_id",
				"msg":    "Unable to store session certificate",
			}.LogC(ctx)
		} else {
			if err := s.CertStore.Store(ctx, parts[0], parts[1], req.SessionCertificate); err != nil {
				alog.Error{
					"action": "AssociatePrimaryUser",
					"status": "certificate_upload_failed",
					"error":  err,
				}.LogC(ctx)
				return nil, err
			}
		}
	} else {
		alog.Warn{
			"action": "AssociatePrimaryUser",
			"status": "missing_session_certificate",
			"msg":    "No robot session certificate was present (non fatal)",
		}.LogC(ctx)
	}

	bundle, err := s.issueToken(ctx, issueTokenArgs{
		action:      "AssociatePrimaryUser",
		revokeJwt:   true,
		issueJwt:    true,
		issueClient: true,
		issueSts:    req.GenerateStsToken,
		tok:         token,
		client:      req.ClientName,
		app:         req.AppId,
	})
	if err != nil {
		return nil, err
	}

	return &tokenpb.AssociatePrimaryUserResponse{
		Data: bundle,
	}, nil
}

func (s *Server) ReassociatePrimaryUser(ctx context.Context, req *tokenpb.ReassociatePrimaryUserRequest) (*tokenpb.ReassociatePrimaryUserResponse, error) {
	requestorId := getCertificateName(ctx)

	// Get the supplied JWT, which can be in any state
	tok := model.GetAccessToken(ctx)

	// Get the credentials from the validated user session
	creds := auth.GetAccountsCredentials(ctx)

	if creds.UserId() != tok.UserId {
		alog.Warn{
			"action": "ReassociatePrimaryUser",
			"status": "user_mismatch",
		}.LogC(ctx)
		return nil, status.Errorf(codes.InvalidArgument, "User does not match primary")
	}

	newToken := makeToken(tok.UserId, requestorId)
	bundle, err := s.issueToken(ctx, issueTokenArgs{
		action:      "ReassociatePrimaryUser",
		revokeJwt:   true,
		issueJwt:    true,
		issueClient: true,
		client:      req.ClientName,
		app:         req.AppId,
		tok:         newToken,
	})
	if err != nil {
		return nil, err
	}

	return &tokenpb.ReassociatePrimaryUserResponse{
		Data: bundle,
	}, nil
}

func (s *Server) AssociateSecondaryClient(ctx context.Context, req *tokenpb.AssociateSecondaryClientRequest) (*tokenpb.AssociateSecondaryClientResponse, error) {
	// Tok will be passed to issueToken() for the sake of logging the
	// RequestorId.
	tok := model.GetAccessToken(ctx)
	if tok == nil {
		// This should really never happen, but is possible during our
		// internal development/rollout/integration phase
		alog.Warn{
			"action": "AssociateSecondaryClient",
			"status": "missing_access_token",
		}.LogC(ctx)
		return nil, status.Errorf(codes.Unauthenticated, "Missing access token")
	}

	if tok.UserId == "" {
		alog.Warn{
			"action": "AssociateSecondaryClient",
			"status": "missing_user_id",
		}.LogC(ctx)
		return nil, status.Errorf(codes.Unauthenticated, "Missing user_id in access token")
	}

	// Get the pointer to the session validator bound by the accounts
	// authenticator interceptor.
	validator := auth.GetSessionValidator(ctx)
	if validator == nil {
		alog.Error{
			"action": "AssociateSecondaryClient",
			"status": "missing_session_validator",
		}.LogC(ctx)
		return nil, status.Errorf(codes.Internal, "No session validator configured")
	}

	// Get the userId from the authentication info provided to the RPC
	// call, which is the userId of the primary user already
	// associated with the robot.
	userId := auth.GetUserId(ctx)
	appkey := grpcutil.GetAppKey(ctx)

	// Validate the secondary session provided in the body of the
	// request.
	credentials, err := validator.ValidateSession(ctx, appkey, req.UserSession)
	if err != nil {
		alog.Warn{
			"action": "AssociateSecondaryClient",
			"status": "session_validation_failed",
			"error":  err,
		}.LogC(ctx)
		return nil, status.Errorf(codes.InvalidArgument, "Secondary session validation failed: %s", err)
	}

	// Check that the secondary session is for the primary user
	if credentials.UserId() != userId {
		alog.Warn{
			"action": "AssociateSecondaryClient",
			"status": "secondary_user_mismatch",
		}.LogC(ctx)
		return nil, status.Errorf(codes.InvalidArgument, "Secondary user does not match primary")
	}

	bundle, err := s.issueToken(ctx, issueTokenArgs{
		action:      "AssociateSecondaryClient",
		issueClient: true,
		issueJwt:    false,
		client:      req.ClientName,
		app:         req.AppId,
		tok:         tok,
	})
	if err != nil {
		return nil, err
	}

	return &tokenpb.AssociateSecondaryClientResponse{
		Data: bundle,
	}, nil
}

func (s *Server) DisassociatePrimaryUser(ctx context.Context, req *tokenpb.DisassociatePrimaryUserRequest) (*tokenpb.DisassociatePrimaryUserResponse, error) {
	tok := model.GetAccessToken(ctx)
	if tok == nil {
		alog.Warn{
			"action": "DisassociatePrimaryUser",
			"status": "token_missing",
		}.LogC(ctx)
		return nil, status.Errorf(codes.Unauthenticated, "Missing access token")
	}
	if err, _ := db.RevokeTokens(ctx, s.Store, db.SearchByRequestor, tok.RequestorId); err != nil {
		return nil, err
	}
	return &tokenpb.DisassociatePrimaryUserResponse{}, nil
}

// wraps the time package for easier testing
var refreshTokenTime = testtime.New()

func (s *Server) RefreshToken(ctx context.Context, req *tokenpb.RefreshTokenRequest) (*tokenpb.RefreshTokenResponse, error) {
	tokenToRefresh := model.GetAccessToken(ctx)

	if tokenToRefresh == nil {
		alog.Warn{
			"action": "RefreshToken",
			"status": "token_missing",
		}.LogC(ctx)
		return nil, status.Errorf(codes.Unauthenticated, "Missing access token")
	}

	// At this point the framework has validated the token's signature
	// and revocation status, so we just need to check if it's inside
	// a valid refresh window.

	// If the token is more than a year old, it's too old to be
	// refreshed.
	if refreshTokenTime.Now().UTC().After(tokenToRefresh.ExpiresAt.Add(TokenRefreshWindowEnd)) {
		alog.Info{
			"action":    "RefreshToken",
			"status":    "token_too_old",
			"token":     tokenToRefresh.Id,
			"issued_at": tokenToRefresh.IssuedAt,
			"user_id":   tokenToRefresh.UserId,
		}.LogC(ctx)
		return nil, status.Errorf(codes.PermissionDenied, "Token too old")
	}

	issueArgs := issueTokenArgs{
		action:      "RefreshToken",
		revokeJwt:   false,
		issueJwt:    false,
		issueSts:    req.RefreshStsTokens,
		issueClient: false,
		tok:         tokenToRefresh,
	}

	// If it's more than 3 hours before the expiration time,
	// just return the original token.
	if refreshTokenTime.Now().UTC().Before(tokenToRefresh.ExpiresAt.Add(-TokenRefreshWindowStart)) {
		alog.Info{
			"action":  "RefreshToken",
			"status":  "token_still_fresh",
			"token":   tokenToRefresh.Id,
			"user_id": tokenToRefresh.UserId,
		}.LogC(ctx)

	} else {
		// Otherwise, we can refresh the token -- generate a new one, and
		// go through the standard issue token process, revoking the old
		// token.
		requestorId := getCertificateName(ctx)
		issueArgs.revokeJwt = true
		issueArgs.issueJwt = true
		issueArgs.tok = makeToken(tokenToRefresh.UserId, requestorId)
	}

	if req.RefreshStsTokens {
		alog.Info{
			"action":  "RefreshToken",
			"status":  "refresh_sts_token",
			"user_id": tokenToRefresh.UserId,
		}.LogC(ctx)
	}

	bundle, err := s.issueToken(ctx, issueArgs)
	if err != nil {
		return nil, err
	}

	if !issueArgs.issueJwt {
		// didn't issue a new JWT token; return the existing
		bundle.Token = tokenToRefresh.Raw
	}

	return &tokenpb.RefreshTokenResponse{
		Data: bundle,
	}, nil
}

func (s *Server) ListRevokedTokens(ctx context.Context, req *tokenpb.ListRevokedTokensRequest) (*tokenpb.ListRevokedTokensResponse, error) {
	// TODO: handle paging input
	if page, err := s.Store.ListRevoked(ctx, nil); err != nil {
		return nil, err
	} else {
		tokens := []string{}
		for _, tok := range page.Tokens {
			if tok.Raw != "" {
				tokens = append(tokens, tok.Raw)
			} else {
				if signed, err := s.Signer.SignToken(tok); err == nil {
					tokens = append(tokens, signed)
				}
			}
		}

		return &tokenpb.ListRevokedTokensResponse{
			Data: &tokenpb.TokensPage{
				Tokens: tokens,
			},
		}, nil
	}
}

func (s *Server) RevokeFactoryCertificate(ctx context.Context, req *tokenpb.RevokeFactoryCertificateRequest) (*tokenpb.RevokeFactoryCertificateResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "RevokeFactoryCertificate not implemented yet.")
}

func (s *Server) RevokeTokens(ctx context.Context, req *tokenpb.RevokeTokensRequest) (*tokenpb.RevokeTokensResponse, error) {
	searchBy := db.SearchByUser
	if req.SearchByIndex == "requestor" {
		searchBy = db.SearchByRequestor
	}
	if err, count := db.RevokeTokens(ctx, s.Store, searchBy, req.Key); err != nil {
		return nil, err
	} else {
		return &tokenpb.RevokeTokensResponse{
			TokensRevoked: uint32(count),
		}, nil
	}
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

// getCertificateName returns the Common Name (CN) of the peer
// certificate (aka the client certificate), if there is one, or
// "unknown" if no peer certificate can be found.
func getCertificateName(ctx context.Context) string {
	if peer := grpcutil.GetPeerCertificate(ctx); peer != nil {
		return peer.Subject.CommonName
	}
	return "unknown"
}

// makeToken constructs a new access token for the give user &
// requestor, with all the fields initialized properly.
func makeToken(userId, requestorId string) *model.Token {
	var (
		iat        = time.Now().UTC()
		expires    = iat.Add(DefaultTokenExpiration)
		tokenId, _ = uuid.NewV4()
	)

	token := &model.Token{
		Id:          tokenId.String(),
		Type:        "user+robot",
		RequestorId: requestorId,
		UserId:      userId,
		IssuedAt:    iat,
		ExpiresAt:   expires,
		RevokedAt:   time.Time{}.UTC(),
	}

	return token
}

var (
	clientTokenTimer          = metricsutil.GetTimer("IssueClientToken.Latency", tokenmetrics.Registry)
	clientTokenSuccessCounter = metricsutil.GetCounter("IssueClientToken.Success", tokenmetrics.Registry)
	clientTokenErrorCounter   = metricsutil.GetCounter("IssueClientToken.Error", tokenmetrics.Registry)
)

// makeClientToken generates a new random client access token,
// returning the plain text password and a salted hash of the
// password.
//
// Returns password, hash, error
func makeClientToken() (string, string, error) {
	start := time.Now()
	defer clientTokenTimer.UpdateSince(start)

	password, hash, err := clienthash.GenerateClientTokenAndHash()
	if err != nil {
		clientTokenErrorCounter.Inc(1)
		return "", "", err
	}
	clientTokenSuccessCounter.Inc(1)
	return password, hash, nil
}

// issueTokenArgs consolidates all the inputs to issueToken(), just to
// keep the code readable without too many positional arguments
type issueTokenArgs struct {
	// action is used in log statements within issueToken()
	action string
	// issueJwt indicates whether or not to store and sign the
	// supplied JWT
	issueJwt bool
	// revokeJwt indicates whether or not to revoke prior JWTs
	revokeJwt bool
	// issueClient indicates whether or not to generate and store a
	// new client token
	issueClient bool
	// revokeClient indicates whether or not to clear all revoke all
	// previously generated client tokens
	revokeClient bool
	// issueSts indicates whether or not to generate an STS token.
	issueSts bool
	// client is a human readable client identifier for the client
	// tokens (such as "Adam's iPhone"), passed in the gRPC call
	client string
	// app is a human readable application identifier for the client
	// tokens (such as "Vector Companion App"), passed in the gRPC
	// call
	app string
	// tok is the JWT to be stored and signed
	tok *model.Token
}

// issueToken handles the mechanics of storing a new token, revoking
// the old tokens for the requestor, and signing the new token. If
// nothing fails, it will return the signed token string.
//
// Returns a TokenBundle protobuf containing the signed access token
// and plain-text client token.
func (s *Server) issueToken(ctx context.Context, args issueTokenArgs) (*tokenpb.TokenBundle, error) {
	if args.revokeJwt {
		if err, _ := db.RevokeTokens(ctx, s.Store, db.SearchByRequestor, args.tok.RequestorId); err != nil {
			alog.Error{
				"action": args.action,
				"status": "revoke_tokens_failed",
				"error":  err,
			}.LogC(ctx)
			return nil, err
		}
	}

	if args.revokeClient {
		if err := s.ClientStore.Clear(ctx, args.tok); err != nil {
			alog.Error{
				"action": args.action,
				"status": "clear_client_tokens_failed",
				"error":  err,
			}.LogC(ctx)
			return nil, err
		}
	}

	bundle := &tokenpb.TokenBundle{}

	if args.issueJwt {
		if err := s.Store.Store(ctx, args.tok); err != nil {
			alog.Error{
				"action": args.action,
				"status": "store_token_failed",
				"error":  err,
			}.LogC(ctx)
			return nil, err
		}

		if signed, err := s.Signer.SignToken(args.tok); err != nil {
			alog.Error{
				"action": args.action,
				"status": "sign_token_failed",
				"error":  err,
			}.LogC(ctx)
			return nil, err
		} else {
			args.tok.Raw = signed
			bundle.Token = signed
		}
	}

	if args.issueSts && s.StsGen.Enabled() {
		if creds, err := s.StsGen.NewToken(ctx, args.tok.UserId, args.tok.RequestorId); err != nil {
			alog.Error{
				"action": args.action,
				"status": "sts_token_failed",
				"error":  err,
			}.LogC(ctx)
			return nil, err
		} else {
			bundle.StsToken = &tokenpb.StsToken{
				AccessKeyId:     aws.StringValue(creds.AccessKeyId),
				SecretAccessKey: aws.StringValue(creds.SecretAccessKey),
				SessionToken:    aws.StringValue(creds.SessionToken),
				Expiration:      creds.Expiration.UTC().Format(time.RFC3339),
			}
		}
	}

	if args.issueClient {
		clientToken, hash, err := makeClientToken()
		if err != nil {
			alog.Error{
				"action": args.action,
				"status": "make_client_token_failed",
				"error":  err,
			}.LogC(ctx)
			return nil, err
		}

		if err := s.ClientStore.Store(ctx, hash, args.tok, args.client, args.app); err != nil {
			alog.Error{
				"action": args.action,
				"status": "store_client_token_failed",
				"error":  err,
			}.LogC(ctx)
			return nil, err
		}

		bundle.ClientToken = clientToken
	}

	return bundle, nil
}
