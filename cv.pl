while(<*.*>) {
	$oldname = $_;
	tr/A-Z/a-z/;
	$newname = $_;
	rename $oldname,$newname;
}
