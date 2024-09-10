import os
import csv
import MetadataKeys
import warnings

class GenerateAudioMetadataSheet:
  
    def generateMetadataFile(self, wwiseIdsData, oldMetadataFileName, outMetadataFileName):

        errorLogStr = []
        # Load Current MetaData CSV file
        oldMetadataCSVData = {}
        if oldMetadataFileName != None and os.path.exists(oldMetadataFileName):
            with open(oldMetadataFileName, 'rb') as csv_file:
                reader = csv.DictReader(csv_file)
                # print "Open Current MetaData CSV Data:"

                for aRow in reader:
                    aRowItem = ItemRowData()
                    aRowItem.setData(aRow)
                    oldMetadataCSVData[aRowItem.wwiseIdKey] = aRowItem

        eventRowData = {}
        notInWWiseEvents = {}

        # Import WWise Ids data
        # Events
        wwiseEventDict = wwiseIdsData["events"]
        hasError = False
        parseEventErrors = []
        for aKey, aVal in wwiseEventDict.iteritems():
            anItemRow = ItemRowData()
            anItemRow.itemType = MetadataKeys.dataTypeKeyEvent
            anItemRow.wwiseIdKey = aKey
            anItemRow.wwiseIdValue = aVal
            # parse wwise key into group and alias names
            if anItemRow.parseWwiseIdIntoGroupAndAlias():
                # Successfully parse add to data
                eventRowData[aKey] = anItemRow
            else:
                # Add Parsing error to Log
                if not hasError:
                    hasError = True
                    parseEventErrors.append("-----------------------------------------------------------")
                    parseEventErrors.append("Audio Event Name Parsing Errors:")
                parseEventErrors.append(" - {} : {}".format(anItemRow.wwiseIdKey, anItemRow.wwiseIdValue))
        if len(parseEventErrors) > 0:
            warnings.warn(os.linesep.join(parseEventErrors))


        # Merge Current Data CSV
        for aRowKey, aRowItem in oldMetadataCSVData.iteritems():
            if aRowKey in eventRowData:
                #    print aRowKey
                # Merge Item data
                # Don't update type, wwise id or wwise value
                eventItem = eventRowData[aRowKey]
                eventItem.audioCategory = aRowItem.audioCategory
                eventItem.groupName = aRowItem.groupName
                eventItem.aliasName = aRowItem.aliasName
                eventItem.comment = aRowItem.comment

            else:
                # Missing Event in WWise
                notInWWiseEvents[aRowKey] = aRowItem

        # Generate new CSV file
        with open(outMetadataFileName, 'wb') as csvfile:
            csv_writer = csv.writer(csvfile)
            # Write Header
            dataHeaderVals = [MetadataKeys.headerKeyType, MetadataKeys.headerKeyAudioCategory,
                              MetadataKeys.headerKeyIdKey, MetadataKeys.headerKeyIdValue,
                              MetadataKeys.headerKeyGroupName, MetadataKeys.headerKeyAliasName,
                              MetadataKeys.headerKeyComment]
            csv_writer.writerow(dataHeaderVals)

            # Write events
            # Sort list using WWise Event Keys
            keys = eventRowData.keys()
            keys.sort()
            for aKey in keys:
                #    print aItemRowData.data()
                anItemRowData = eventRowData[aKey]
                csv_writer.writerow(anItemRowData.data())

            # Print Missing events to worn user
            if len(notInWWiseEvents) > 0:
                keys = notInWWiseEvents.keys()
                keys.sort()
                errorLogStr.append("-----------------------------------------------------------")
                errorLogStr.append("Audio Events were removed please verify changes.")
                for aKey in keys:
                    anItemRowData = notInWWiseEvents[aKey]
                    errorLogStr.append(" - %s : %s" % (anItemRowData.wwiseIdKey, anItemRowData.wwiseIdValue))
              
        return errorLogStr




    def readMetadataSheet(self, metadataFileName):
        with open(metadataFileName, 'rb') as csv_file:
            reader = csv.DictReader(csv_file)
            data = []

            for aRow in reader:
                rowData = ItemRowData()
                rowData.setData(aRow)

                # Skip if there is no Type
                if rowData.itemType == '':
                    continue

                # Otherwise add data
                data.append(rowData)  # CSV Row data

            return data




class ItemRowData:
    itemType = ""
    audioCategory = ""
    wwiseIdKey = ""
    wwiseIdValue = 0
    groupName = ""
    aliasName = ""
    comment = ""

    def __int__(self, dataDict):
        self.setData(dataDict)

    def setData(self, dataDict):
        self.itemType = dataDict[MetadataKeys.headerKeyType]
        self.audioCategory = dataDict[MetadataKeys.headerKeyAudioCategory]
        self.wwiseIdKey = dataDict[MetadataKeys.headerKeyIdKey]
        self.wwiseIdValue = dataDict[MetadataKeys.headerKeyIdValue]
        self.groupName = dataDict[MetadataKeys.headerKeyGroupName]
        self.aliasName = dataDict[MetadataKeys.headerKeyAliasName]
        self.comment = dataDict[MetadataKeys.headerKeyComment]

    def data(self):
        return [self.itemType, self.audioCategory, self.wwiseIdKey, self.wwiseIdValue, self.groupName, self.aliasName,
                self.comment]

    def jsonData(self):
        return { 'type' : self.itemType, 'category' : self.audioCategory, 'wwiseName' : self.wwiseIdKey,
                 'wwiseIdValue' : self.wwiseIdValue, 'groupName' : self.groupName, 'aliasName' : self.aliasName,
                 'comment' : self.comment }

    def parseWwiseIdIntoGroupAndAlias(self):
        success = False
        # Expected format: <action_type>__<group_name>__<event_name>
        # Split string, and assign the group name and alias name
        if self.wwiseIdKey != "":
            # Check if it's in the correct format
            items = self.wwiseIdKey.split("__")
            if len(items) == 3:
                self.groupName = items[1]
                self.aliasName = items[2]
                success = True
        return success
