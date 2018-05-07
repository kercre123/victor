from __future__ import print_function # Python 2/3 compatibility
import boto3

dynamodb = boto3.resource('dynamodb', region_name='us-east-2')


table = dynamodb.create_table(
    TableName='FileInfo',
    KeySchema=[
        {
            'AttributeName': 'name',
            'KeyType': 'HASH'  #Partition key
        },
        {
            'AttributeName': 'date',
            'KeyType': 'RANGE'  #Sort key
        }
        # {
        #     'AttributeName': 'age',
        #     'KeyType': 'RANGE'  #Sort key
        # },
        # {
        #     'AttributeName': 'gender',
        #     'KeyType': 'RANGE'  #Sort key
        # },
        # {
        #     'AttributeName': 'distance',
        #     'KeyType': 'RANGE'  #Sort key
        # },
        # {
        #     'AttributeName': 'dbpath',
        #     'KeyType': 'RANGE'  #Sort key
        # }
    ],
    AttributeDefinitions=[
        {
            'AttributeName': 'name',
            'AttributeType': 'S'
        },
        {
            'AttributeName': 'date',
            'AttributeType': 'S'
        }
        # {
        #     'AttributeName': 'age',
        #     'AttributeType': 'S'
        # },
        # {
        #     'AttributeName': 'gender',
        #     'AttributeType': 'S'
        # },
        # {
        #     'AttributeName': 'distance',
        #     'AttributeType': 'S'
        # },
        # {
        #     'AttributeName': 'dbpath',
        #     'AttributeType': 'S'
        # }

    ],
    ProvisionedThroughput={
        'ReadCapacityUnits': 2,
        'WriteCapacityUnits': 2
    }
)

print("Table status:", table.table_status)