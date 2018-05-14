from __future__ import print_function # Python 2/3 compatibility
import boto3
import json
import decimal
from boto3.dynamodb.conditions import Key, Attr
import settings

class DynamoDB():

    DATABASE = 'dynamodb'
    REGION = settings.REGION_NAME
    TABLE = settings.TABLE_NAME

    ITEM = u'Items'

    json_data = ""
    dynamodb = boto3.resource(DATABASE, region_name=REGION)
    table = dynamodb.Table(TABLE)

    def __init__(self, json_data):
        self.json_data = json.dumps(json_data)

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
        items = json.loads(self.json_data, parse_float = decimal.Decimal)
        self.putItemToTable(self.table, items)
        print("Done")

    def filterValues(self):
        datas = None
        for key, value in json.loads(self.json_data, parse_float = decimal.Decimal).items():
            if datas == None:
                datas = Key(key).eq(value)
            else:
                datas = datas & Key(key).eq(value)
        response = self.table.scan(FilterExpression=datas)
        for i in response[self.ITEM]:
            print(json.dumps(i, cls=DecimalEncoder))
        return response[self.ITEM]

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
        obj = super(DecimalEncoder, self).default(o)
        if isinstance(o, decimal.Decimal):
            if o % 1 > 0:
                obj = float(o)
            else:
                obj = int(o)
        return obj
