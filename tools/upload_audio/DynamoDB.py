from __future__ import print_function # Python 2/3 compatibility
import boto3
import json
import decimal
from boto3.dynamodb.conditions import Key, Attr

class DynamoDB():

    JSON_DATA = ""
    dynamodb = boto3.resource('dynamodb', region_name='us-east-2')
    table = dynamodb.Table('FileInfo')

    def __init__(self, json_data):
        self.JSON_DATA = json.dumps(json_data)

    def putItemToTable(self, table, item):
        table.put_item(Item={
                            'name': item['name'],
                            'age': item['age'],
                            'distance': item['distance'],
                            'dropbox_path': item['dropbox_path'],
                            'date': item['date'],
                            'gender': item['gender']
            }
        )

    def putItem(self):
        items = json.loads(self.JSON_DATA, parse_float = decimal.Decimal)
        self.putItemToTable(self.table, items)
        print("Done")

    def filterValues(self):
        datas = None
        for key, value in json.loads(self.JSON_DATA, parse_float = decimal.Decimal).items():
            if datas == None:
                datas = Key(key).eq(value)
            else:
                datas = datas & Key(key).eq(value)
        response = self.table.scan(FilterExpression=datas)
        for i in response[u'Items']:
            print(json.dumps(i, cls=DecimalEncoder))
        return response[u'Items']

    # Helper compare json
    def ordered(obj):
        if isinstance(obj, dict):
            return sorted((k, ordered(v)) for k, v in obj.items())
        if isinstance(obj, list):
            return sorted(ordered(x) for x in obj)
        else:
            return obj


# Helper class to convert a DynamoDB item to JSON.
class DecimalEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, decimal.Decimal):
            if o % 1 > 0:
                return float(o)
            else:
                return int(o)
        return super(DecimalEncoder, self).default(o)
