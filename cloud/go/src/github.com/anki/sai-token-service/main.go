package main

import (
	"os"

	"github.com/anki/sai-service-framework/svc"
	"github.com/anki/sai-token-service/commands"
	"github.com/anki/sai-token-service/commands/tokencmd"
	"github.com/anki/sai-token-service/server/token"
)

func main() {
	svc.ServiceMain(os.Args[0], "Token Management Service",
		commands.CreateTables(),
		commands.CheckTables(),
		commands.UpdateTTL(),
		tokencmd.AssociatePrimaryUser(),     // test
		tokencmd.AssociateSecondaryClient(), // test
		tokencmd.RefreshToken(),             // test
		tokencmd.ListRevokedTokens(),        // test
		svc.WithServiceCommand("start", "Run the RPC service", token.NewService()))
}
