#
# rename.pl
while(<*>) {
	rename($_,lc($_));
}
