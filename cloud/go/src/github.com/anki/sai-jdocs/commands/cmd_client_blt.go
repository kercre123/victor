package commands

// BLT = Baby Load Test

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"time"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-jdocs/jdocs"
	"github.com/anki/sai-jdocs/jdocs/db"   // package jdocsdb
	"github.com/anki/sai-jdocs/jdocs/test" // package jdocstest
	"github.com/anki/sai-jdocs/proto/jdocspb"
	"github.com/anki/sai-service-framework/grpc/svc"
	"github.com/anki/sai-service-framework/svc"

	"github.com/jawher/mow.cli"
	"google.golang.org/grpc"
)

const (
	BLTFmtVersion = 1
)

func NewBabyLoadTestClientCommand() *grpcsvc.ClientCommand {
	var deleteFirst *bool
	var docSpecsFile *string
	var numEntities, numDataStrings, numIterations, numItrPerUpdate, periodMillisecs *int
	return grpcsvc.NewClientCommand(
		"jdocs", // prefix for args and env variables
		"[--delete-first] [--specs-file] [--num-entities] [--num-data-strings] [--num-iterations] [--num-itr-per-update] [--period-ms]", // spec
		func(cmd *cli.Cmd) { // init
			deleteFirst = svc.BoolOpt(cmd, "delete-first", false, "When true, force delete all documents that will be created for load test")
			docSpecsFile = svc.StringOpt(cmd, "f specs-file", "", "Filename of document specs (JSON format)")
			numEntities = svc.IntOpt(cmd, "e num-entities", 2, "Number of independent Accounts, Things")
			numDataStrings = svc.IntOpt(cmd, "s num-data-strings", 2, "Number of data strings in each document (to specify coarse size of documents)")
			numIterations = svc.IntOpt(cmd, "i num-iterations", 2, "Number iterations for the load test")
			numItrPerUpdate = svc.IntOpt(cmd, "u num-itr-per-update", 5, "Number iterations per update (ie the doc read-to-write ratio)")
			periodMillisecs = svc.IntOpt(cmd, "p period-ms", 500, "Update iteration period: start-to-start milliseconds")
		},
		func(conn *grpc.ClientConn) error { // action
			client := jdocspb.NewJdocsClient(conn)

			// Read and parse the document specs (from JSON file)
			docSpecsJson := []byte{}
			if *docSpecsFile == "" {
				return fmt.Errorf("A doc specs file must be specified")
			} else {
				if jsonBytes, rerr := ioutil.ReadFile(*docSpecsFile); rerr != nil {
					return rerr
				} else {
					docSpecsJson = jsonBytes
				}
			}
			var docSpecList jdocs.JdocSpecList
			if jerr := json.Unmarshal(docSpecsJson, &docSpecList); jerr != nil {
				return fmt.Errorf("Cannot parse JSON for JDOCS document specs: %q", jerr)
			}

			if *deleteFirst {
				fmt.Println("Deleting all documents")
				if derr := BLTActOnDocs(&docSpecList, client, *numEntities, *numDataStrings, 0, BLTDeleteDocFunc); derr != nil {
					alog.Error{
						"action": "delete_docs",
						"status": "Error",
						"error":  derr,
					}.Log()
				}
			}

			fmt.Println("Creating the first version of all documents")
			if cerr := BLTActOnDocs(&docSpecList, client, *numEntities, *numDataStrings, 0, BLTCreateDocFunc); cerr != nil {
				alog.Error{
					"action": "create_docs",
					"status": "Error",
					"error":  cerr,
				}.Log()
				return cerr
			}

			ticker := time.NewTicker(time.Duration(*periodMillisecs) * time.Millisecond)
			for itr := 0; itr < *numIterations; itr++ {
				<-ticker.C
				expDocVersion := uint64((itr / *numItrPerUpdate) + 1)
				fmt.Printf("Iteration %d/%d (expDocVersion=%d)\n", itr+1, *numIterations, expDocVersion)

				// Indivdually read and validate documents
				BLTActOnDocs(&docSpecList, client, *numEntities, *numDataStrings, expDocVersion, BLTVerifyDocFunc)

				// Batch-read documents, then validate
				BLTVerifyAcctThngDocs(&docSpecList, client, *numEntities, *numDataStrings, expDocVersion)

				if (itr % *numItrPerUpdate) == (*numItrPerUpdate - 1) {
					// Update documents
					BLTActOnDocs(&docSpecList, client, *numEntities, *numDataStrings, expDocVersion, BLTUpdateDocFunc)
				}
			}

			fmt.Println("Deleting all documents")
			if derr := BLTActOnDocs(&docSpecList, client, *numEntities, *numDataStrings, 0, BLTDeleteDocFunc); derr != nil {
				alog.Error{
					"action": "delete_docs",
					"status": "Error",
					"error":  derr,
				}.Log()
				return derr
			}

			return nil
		})
}

//////////////////////////////////////////////////////////////////////

// BLTVerifyAcctThngDocs iterates through a set of Accounts a[1]..a[numEntities]
// and a set of Things t[1]..t[numEntities], then does a read-verify on all
// AccountThing documents. ReadDocs() is used to efficiently batch-read all
// documents associated with an account.
func BLTVerifyAcctThngDocs(docSpecs *jdocs.JdocSpecList, client jdocspb.JdocsClient, numEntities, numDataStrings int, docVersion uint64) {
	for acct := 0; acct < numEntities; acct++ {
		for thng := 0; thng < numEntities; thng++ {
			// Make list of documents to read
			rreq := jdocspb.ReadDocsReq{
				Account: jdocstest.AccountName(acct + 1),
				Thing:   jdocstest.ThingName(thng + 1),
				Items:   make([]*jdocspb.ReadDocsReq_Item, 0),
			}
			for _, docSpec := range docSpecs.Specs {
				if jdocsdb.TableDocType(docSpec.TableName) == jdocsdb.DocTypeAccountThing {
					rreq.Items = append(rreq.Items, &jdocspb.ReadDocsReq_Item{
						DocName:      docSpec.DocName,
						MyDocVersion: docVersion - 1,
					})
				}
			}
			// Read documents
			rresp, err := client.ReadDocs(context.Background(), &rreq)
			if err != nil {
				alog.Error{
					"action": "read_docs",
					"status": "Error",
					"error":  err,
				}.Log()
			}
			if len(rresp.Items) != len(rreq.Items) {
				alog.Error{
					"action": "read_docs",
					"status": "Error",
					"error":  fmt.Sprintf("ReadDocsResp item length mismatch. Exp=%d, got=%d", len(rreq.Items), len(rresp.Items)),
				}.Log()
				return
			}
			// Verify document contents
			for i, reqItem := range rreq.Items {
				expDoc, err := jdocstest.NewJdoc(jdocstest.DocKey{
					Account:        rreq.Account,
					Thing:          rreq.Thing,
					DocName:        reqItem.DocName,
					DocType:        jdocsdb.DocTypeAccountThing,
					DocVersion:     docVersion,
					FmtVersion:     BLTFmtVersion,
					NumDataStrings: numDataStrings,
				})
				if err != nil {
					alog.Error{
						"action": "make_exp_doc",
						"status": "Error",
						"error":  err,
					}.Log()
					return // should never happen
				}
				gotDoc := rresp.Items[i].Doc
				if *gotDoc != *expDoc {
					alog.Error{
						"action": "verify_doc",
						"status": "Error",
						"error":  "Document mismatch",
						"exp":    expDoc,
						"got":    gotDoc,
					}.Log()
				}
			}
		}
	}
}

// BLTActOnDocs iterates through a set of Accounts a[1]..a[numEntities] and a
// set of Things t[1]..t[numEntities], then performs some action on all
// permutations of Account/Thing/AccountThing documents for those sets of
// Accounts and Things.
func BLTActOnDocs(docSpecs *jdocs.JdocSpecList, client jdocspb.JdocsClient, numEntities, numDataStrings int, docVersion uint64, actFn func(jdocspb.JdocsClient, jdocstest.DocKey) error) error {
	for _, docSpec := range docSpecs.Specs {
		key := jdocstest.DocKey{
			DocName:        docSpec.DocName,
			DocType:        jdocsdb.TableDocType(docSpec.TableName),
			DocVersion:     docVersion,
			FmtVersion:     BLTFmtVersion,
			NumDataStrings: numDataStrings,
		}
		docType := jdocsdb.TableDocType(docSpec.TableName)
		switch docType {
		case jdocsdb.DocTypeAccount:
			for acct := 0; acct < numEntities; acct++ {
				key.Account = jdocstest.AccountName(acct + 1)
				key.Thing = ""
				err := actFn(client, key)
				if err != nil {
					return err
				}
			}
		case jdocsdb.DocTypeThing:
			for thng := 0; thng < numEntities; thng++ {
				key.Account = ""
				key.Thing = jdocstest.ThingName(thng + 1)
				err := actFn(client, key)
				if err != nil {
					return err
				}
			}
		case jdocsdb.DocTypeAccountThing:
			for acct := 0; acct < numEntities; acct++ {
				for thng := 0; thng < numEntities; thng++ {
					key.Account = jdocstest.AccountName(acct + 1)
					key.Thing = jdocstest.ThingName(thng + 1)
					err := actFn(client, key)
					if err != nil {
						return err
					}
				}
			}
		}
	}
	return nil
}

//////////////////////////////////////////////////////////////////////

func BLTCreateDocFunc(client jdocspb.JdocsClient, key jdocstest.DocKey) error {
	// make sure document doesn't already exist
	rresp, rerr := client.ReadDocs(context.Background(),
		&jdocspb.ReadDocsReq{
			Account: key.Account,
			Thing:   key.Thing,
			Items: []*jdocspb.ReadDocsReq_Item{
				&jdocspb.ReadDocsReq_Item{
					DocName:      key.DocName,
					MyDocVersion: key.DocVersion,
				},
			},
		})
	if rerr != nil {
		return rerr
	}
	if len(rresp.Items) != 1 {
		return fmt.Errorf("BLTCreateDocFunc: Wrong number of items in ReadDocs response for key=%v. Exp=1, got=%d", key, len(rresp.Items))
	}

	if gotDocVersion := rresp.Items[0].Doc.DocVersion; gotDocVersion != 0 {
		return fmt.Errorf("BLTCreateDocFunc: Wrong DocVersion for key=%v: Exp=0, got=%d", key, gotDocVersion)
	}

	// create doc
	key.DocVersion = 1 // XXX
	doc, err := jdocstest.NewJdoc(key)
	doc.DocVersion = 0
	key.DocVersion = 0
	if err != nil {
		return err
	}
	wresp, werr := client.WriteDoc(context.Background(),
		&jdocspb.WriteDocReq{
			Account: key.Account,
			Thing:   key.Thing,
			DocName: key.DocName,
			Doc:     doc,
		})
	if werr != nil {
		return werr
	}
	if wresp.Status != jdocspb.WriteDocResp_ACCEPTED {
		err := fmt.Errorf("WriteDocResp.Status mismatch. Exp=%s, got=%s", jdocspb.WriteDocResp_ACCEPTED, wresp.Status)
		alog.Error{
			"action": "create_doc",
			"status": "Error",
			"error":  err,
		}.Log()
		return err
	}
	if wresp.LatestDocVersion != 1 {
		err := fmt.Errorf("WriteDocResp.LatestDocVersion mismatch. Exp=1, got=%d", wresp.LatestDocVersion)
		alog.Error{
			"action": "create_doc",
			"status": "Error",
			"error":  err,
		}.Log()
		return err
	}
	return nil
}

func BLTVerifyDocFunc(client jdocspb.JdocsClient, key jdocstest.DocKey) error {
	// read the document
	rresp, rerr := client.ReadDocs(context.Background(),
		&jdocspb.ReadDocsReq{
			Account: key.Account,
			Thing:   key.Thing,
			Items: []*jdocspb.ReadDocsReq_Item{
				&jdocspb.ReadDocsReq_Item{
					DocName:      key.DocName,
					MyDocVersion: 0,
				},
			},
		})
	if rerr != nil {
		return rerr
	}
	if len(rresp.Items) != 1 {
		return fmt.Errorf("BLTVerifyDocFunc: Wrong number of items in ReadDocs response for key=%v. Exp=1, got=%d", key, len(rresp.Items))
	}

	// Verify document contents
	expDoc, err := jdocstest.NewJdoc(key)
	if err != nil {
		alog.Error{
			"action": "make_exp_doc",
			"status": "Error",
			"error":  err,
		}.Log()
		return err // should never happen
	}
	gotDoc := rresp.Items[0].Doc
	if *gotDoc != *expDoc {
		alog.Error{
			"action": "verify_doc",
			"status": "Error",
			"error":  "Document mismatch",
			"exp":    expDoc,
			"got":    gotDoc,
		}.Log()
	}

	return nil
}

func BLTUpdateDocFunc(client jdocspb.JdocsClient, key jdocstest.DocKey) error {
	// Update that should be rejected
	key.DocVersion++ // XXX
	doc, err := jdocstest.NewJdoc(key)
	doc.DocVersion = key.DocVersion - 2 // should cause JDOCS to reject the document
	if err != nil {
		return err
	}
	wreq := jdocspb.WriteDocReq{
		Account: key.Account,
		Thing:   key.Thing,
		DocName: key.DocName,
		Doc:     doc,
	}
	wresp, werr := client.WriteDoc(context.Background(), &wreq)
	if werr != nil {
		return werr
	}
	if wresp.Status != jdocspb.WriteDocResp_REJECTED_BAD_DOC_VERSION {
		err := fmt.Errorf("WriteDocResp.Status mismatch. Exp=%s, got=%s\nwreq=%v", jdocspb.WriteDocResp_REJECTED_BAD_DOC_VERSION, wresp.Status, wreq)
		alog.Error{
			"action": "write_doc",
			"status": "Error",
			"error":  err,
		}.Log()
		return err
	}

	// Update that should be accepted
	doc, err = jdocstest.NewJdoc(key)
	doc.DocVersion = key.DocVersion - 1 // should cause JDOCS to accept the document
	if err != nil {
		return err
	}
	wreq = jdocspb.WriteDocReq{
		Account: key.Account,
		Thing:   key.Thing,
		DocName: key.DocName,
		Doc:     doc,
	}
	wresp, werr = client.WriteDoc(context.Background(), &wreq)
	if werr != nil {
		return werr
	}
	if wresp.Status != jdocspb.WriteDocResp_ACCEPTED {
		err := fmt.Errorf("WriteDocResp.Status mismatch. Exp=%s, got=%s\nwreq=%v", jdocspb.WriteDocResp_ACCEPTED, wresp.Status, wreq)
		alog.Error{
			"action": "write_doc",
			"status": "Error",
			"error":  err,
		}.Log()
		return err
	}
	if wresp.LatestDocVersion != key.DocVersion {
		err := fmt.Errorf("WriteDocResp.LatestDocVersion mismatch. Exp=%d, got=%d", key.DocVersion, wresp.LatestDocVersion)
		alog.Error{
			"action": "write_doc",
			"status": "Error",
			"error":  err,
		}.Log()
		return err
	}
	return nil
}

func BLTDeleteDocFunc(client jdocspb.JdocsClient, key jdocstest.DocKey) error {
	_, derr := client.DeleteDoc(context.Background(),
		&jdocspb.DeleteDocReq{
			Account: key.Account,
			Thing:   key.Thing,
			DocName: key.DocName,
		})
	if derr != nil {
		return derr
	}
	return nil
}
