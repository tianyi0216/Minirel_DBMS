
"""
FILE: skeleton_parser.py
------------------
Author: Firas Abuzaid (fabuzaid@stanford.edu)
Author: Perth Charernwattanagul (puch@stanford.edu)
Modified: 04/21/2014

Skeleton parser for CS564 programming project 1. Has useful imports and
functions for parsing, including:

1) Directory handling -- the parser takes a list of eBay json files
and opens each file inside of a loop. You just need to fill in the rest.
2) Dollar value conversions -- the json files store dollar value amounts in
a string like $3,453.23 -- we provide a function to convert it to a string
like XXXXX.xx.
3) Date/time conversions -- the json files store dates/ times in the form
Mon-DD-YY HH:MM:SS -- we wrote a function (transformDttm) that converts to the
for YYYY-MM-DD HH:MM:SS, which will sort chronologically in SQL.

Your job is to implement the parseJson function, which is invoked on each file by
the main function. We create the initial Python dictionary object of items for
you; the rest is up to you!
Happy parsing!
"""

import sys
from json import loads
from re import sub

columnSeparator = "|"

# Dictionary of months used for date transformation
MONTHS = {'Jan':'01','Feb':'02','Mar':'03','Apr':'04','May':'05','Jun':'06',\
        'Jul':'07','Aug':'08','Sep':'09','Oct':'10','Nov':'11','Dec':'12'}

"""
Returns true if a file ends in .json
"""
def isJson(f):
    return len(f) > 5 and f[-5:] == '.json'

"""
Converts month to a number, e.g. 'Dec' to '12'
"""
def transformMonth(mon):
    if mon in MONTHS:
        return MONTHS[mon]
    else:
        return mon

"""
Transforms a timestamp from Mon-DD-YY HH:MM:SS to YYYY-MM-DD HH:MM:SS
"""
def transformDttm(dttm):
    dttm = dttm.strip().split(' ')
    dt = dttm[0].split('-')
    date = '20' + dt[2] + '-'
    date += transformMonth(dt[0]) + '-' + dt[1]
    return date + ' ' + dttm[1]

"""
Transform a dollar value amount from a string like $3,453.23 to XXXXX.xx
"""

def transformDollar(money):
    if money == None or len(money) == 0:
        return money
    return sub(r'[^\d.]', '', money)

"""
Parses a single json file. Currently, there's a loop that iterates over each
item in the data set. Your job is to extend this functionality to create all
of the necessary SQL tables for your database.
"""
def parseJson(json_file):
    with open(json_file, 'r') as f:
        # put all items into lists and then just join them on a pipe
        itemData = loads(f.read())['Items'] # creates a Python dictionary of Items for the supplied json file
        bids = set()
        bidPK = set()
        
        categories = set()
        categoriesPK = set()
        
        items = set()
        itemsPK = set()
        
        auction = set()
        auctionPK = set()
        
        users = set()
        userPK = set()

        auctionString = ""
        itemString = ""
        buyerString = ""
        userString = ""

        # Item schema (IID, Name, Buy_Price, FirstBid, Description) PK(IID)
        # Bid schema(IID, BID, BidTime, BidAmount) PK(IID, BID, BidTime)
        # Auction schema(IID, SID, Curr,  Num_Bids, StartDate, EndDate) PK(IID, SID)
        # Category schema(IID, Category) PK(IID, Category)
        # User schema(ID, Rating, Country, Location) PK(ID)
        for item in itemData:
            if(f'{item["ItemID"]}|' not in itemsPK):
                # Create item string
                itemString += (item['ItemID'] + "|") # IID
                itemString += ("\"" + item['Name'].replace('"','""') + "\"|") # Name
                itemString += (transformDollar(item['Buy_Price']) + "|") if 'Buy_Price' in item.keys() else "NULL|" # Buy_Price if present else NULL
                itemString += transformDollar(item['First_Bid']) + "|" # First bid
                itemString += ("\"" + item['Description'].replace('"','""') +"\"\n") if ('Description' in item.keys() and item['Description'] != None) else "NULL\n" # Description
                items.add(itemString)
                itemsPK.add(item['ItemID'] + "|")
                itemString = ""

            check = item['ItemID'] + "|" + "\"" + item['Seller']['UserID'].replace('"','""') +"\"|"
            if(check not in auctionPK):
            # create auction string
                auctionString += (item['ItemID'] + "|") # IID
                auctionString += ("\"" + item['Seller']['UserID'].replace('"','""') +"\"|") # SID
                auctionString += transformDollar(item['Currently']) + "|" # Curr_price
                auctionString += item['Number_of_Bids'] + "|" # Num bids
                auctionString += transformDttm(item['Started']) + "|" # Auction start
                auctionString += transformDttm(item['Ends']) + "\n" # Auction end
                auction.add(auctionString)
                auctionPK.add(item['ItemID'] + "|" + "\"" + item['Seller']['UserID'].replace('"','""') +"\"|")
                auctionString = ""

            if(("\"" + item['Seller']['UserID'].replace('"','""') +"\"|") not in userPK):
                userString += ("\"" + item['Seller']['UserID'].replace('"','""') +"\"|") # SID
                userString += item['Seller']['Rating'] + "|" # Rating
                userString += ("\"" + item['Location'].replace('"','""') + "\"|") if 'Location' in item.keys() else "NULL|" # Location if present else NULL
                userString += ("\"" + item['Country'].replace('"','""') + "\"\n") if 'Country' in item.keys() else "NULL\n" # Country if present else NULL
                users.add(userString)
                userPK.add("\"" + item['Seller']['UserID'].replace('"','""') +"\"|")
                userString = ""
            
            # goes through item's categories and adds them to category table
            for cat in item['Category']:
                if f'{item["ItemID"]}|\"{cat}\"' not in categoriesPK:
                    categories.add(f'{item["ItemID"]}|\"{cat}\"\n')
                    categoriesPK.add(f'{item["ItemID"]}|\"{cat}\"')

            # check to see if anyone has bidded
            if(int(item['Number_of_Bids']) > 0):
                # loop through bidders to make strings and find highest bidder
                for biddict in item['Bids']:
                    # get bidder object
                    bidder = biddict['Bid']['Bidder']
                    check = item['ItemID'] + "|" + "\"" + bidder['UserID'].replace('"','""') + "\"|" + transformDttm(biddict['Bid']['Time'])
                    if check not in bidPK:
                        buyerString += ("\"" + bidder['UserID'].replace('"','""') + "\"|") # BID
                        buyerString += item['ItemID'] + "|"
                        buyerString += (transformDttm(biddict['Bid']['Time'])) + "|" # Bid time
                        buyerString += (transformDollar(biddict['Bid']['Amount']) + "\n") # Bid amount
                        bids.add(buyerString)
                        bidPK.add(check)
                        buyerString = ""
                        
                    if ("\"" + bidder['UserID'].replace('"','""') + "\"|") not in userPK:
                        userString += ("\"" + bidder['UserID'].replace('"','""') + "\"|")
                        userString += (bidder['Rating'] + "|") # Rating
                        userString += ("\"" + bidder['Location'].replace('"', '""') + "\"|") if 'Location' in bidder.keys() else "NULL|" # Location if present else NULL
                        userString += ("\"" + bidder['Country'].replace('"', '""') + "\"\n") if 'Country' in bidder.keys() else "NULL\n" # Country if present else NULL
                        users.add(userString)
                        userPK.add("\"" + bidder['UserID'].replace('"','""') + "\"|")
                        userString = ""
        # print(f'Items:\n{items}\n')
        # print(f'Buyers:\n{buyers}\n')
        # print(f'Auction:\n{auction}\n')
        # print(f'Categories:\n{categories}\n')
        return items, bids, auction, categories, users


"""
Loops through each json files provided on the command line and passes each file
to the parser
"""
def main(argv):
    if len(argv) < 2:
        print(sys.stderr, 'Usage: python skeleton_json_parser.py <path to json files>')
        sys.exit(1)
    # loops over all .json files in the argument
    for file in argv[1:]:
        if isJson(file):
            items, bids, auctions, categories, users = parseJson(file)
            with open('itemsTest.dat', 'a') as f:
                for item in items:
                    f.write(item)
            f.close()
            with open('bidTest.dat', 'a') as f:
                for bid in bids:
                    f.write(bid)
            f.close()
            with open('auctionsTest.dat', 'a') as f:
                for auction in auctions:
                    f.write(auction)
            f.close()
            with open('categoriesTest.dat', 'a') as f:
                for category in categories:
                    f.write(category)
            f.close()
            with open('userTest.dat', 'a') as f:
                for user in users:
                    f.write(user)
            f.close()

            print(f'Success parsing {file}')

if __name__ == '__main__':
    main(sys.argv)
