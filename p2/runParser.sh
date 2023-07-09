rm *.dat

python ./skeleton_parser.py ./ebay_data/items-*.json

sort auctionsTest.dat | uniq > auctions.dat
sort bidTest.dat | uniq > bids.dat
sort userTest.dat | uniq > users.dat
sort itemsTest.dat | uniq  > items.dat
sort categoriesTest.dat | uniq > categories.dat

rm auctionsTest.dat
rm bidTest.dat
rm userTest.dat
rm itemsTest.dat
rm categoriesTest.dat