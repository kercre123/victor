package main

import (
	"fmt"
	"strings"

	"github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/dynamoutil"
	"github.com/aws/aws-sdk-go/service/dynamodb"
)

var listTablesCmd = &Command{
	Name:       "list-tables",
	RequiresDb: true,
	Exec: func(cmd *Command, args []string) {
		setupDb(false)
		out, err := dynamoutil.DynamoDB.ListTables(&dynamodb.ListTablesInput{})
		if err != nil {
			fmt.Println("Failed: ", err)
			cli.Exit(1)
		}
		if len(out.TableNames) == 0 {
			fmt.Println("No tables found")
			cli.Exit(1)
		}

		for _, name := range out.TableNames {
			if strings.HasPrefix(*name, dynamoutil.TablePrefix) {
				out, err := dynamoutil.DynamoDB.DescribeTable(&dynamodb.DescribeTableInput{
					TableName: name,
				})
				if err != nil {
					fmt.Printf("Failed to get table description for table %q: %s", *name, err)
					cli.Exit(1)
				}
				desc := out.Table
				fmt.Printf("%s\n", desc)
			}
		}
	},
}
