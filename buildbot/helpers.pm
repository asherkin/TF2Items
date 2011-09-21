#!/usr/bin/perl

use strict;
use Cwd;

package Build;

sub HgRevNum
{
	my ($path) = (@_);
	my ($cd, $text, $rev);

	$cd = Cwd::cwd();
	chdir($path);
	$text = `hg identify -n`;
	chdir($cd);

	chomp $text;
	if ($text =~ /^(\d+)/)
	{
		return $1;
	}

	return 0;
}

sub ProductVersion
{
	my ($file) = (@_);
	my ($version);
	open(FILE, $file) or die "Could not open $file: $!\n";
	$version = <FILE>;
	close(FILE);
	chomp $version;
	return $version;
}

sub Delete
{
	my ($str)=(@_);
	if ($^O =~ /MSWin/)
	{
		Command("del /S /F /Q \"$str\"");
		Command("rmdir /S /Q \"$str\"");
	} else {
		Command("rm -rf $str");
	}
	return !(-e $str);
}

sub Copy
{
	my ($src,$dest)=(@_);
	if ($^O =~ /MSWin/)
	{
		Command("copy \"$src\" \"$dest\" /y");
	} else {
		Command("cp \"$src\" \"$dest\"");
	}
	return (-e $dest);
}

sub Move
{
	my ($src,$dest)=(@_);
	if ($^O =~ /MSWin/)
	{
		Command("move \"$src\" \"$dest\"");
	} else {
		Command("mv \"$src\" \"$dest\"");
	}
	return (-e $dest);
}

sub Command
{
	my($cmd)=(@_);
	print "$cmd\n";
	return `$cmd`;
}

sub PathFormat
{
	my ($str)=(@_);
	if ($^O =~ /MSWin/)
	{
		$str =~ s#/#\\#g;
	} else {
		$str =~ s#\\#/#g;
	}
	return $str;
}

return 1;
