#!/usr/bin/perl

use strict;
use Cwd;
use File::Basename;
use Net::FTP;

my ($ftp_host, $ftp_user, $ftp_pass, $ftp_path);

$ftp_host = $ARGV[0];
$ftp_user = $ARGV[1];
$ftp_pass = $ARGV[2];
$ftp_path = $ARGV[3];

my ($myself, $path) = fileparse($0);
chdir($path);

require 'helpers.pm';

my ($version);
$version = Build::ProductVersion(Build::PathFormat('product.version'));
$version .= '-hg' . Build::HgRevNum('.');

# Append OS to package version
if ($^O eq "darwin")
{
    $version .= '-mac';
}
elsif ($^O =~ /MSWin/)
{
    $version .= '-windows';
}
else
{
    $version .= '-' . $^O;
}

#Switch to the output folder.
chdir(Build::PathFormat('../../OUTPUT/package'));

my ($filename);
$filename = 'tf2items-' . $version;
if ($^O eq "linux")
{
    $filename .= '.tar.gz';
    print "tar zcvf $filename addons\n";
    system("tar zcvf $filename addons");
}
else
{
    $filename .= '.zip';
    print "zip -r $filename addons\n";
    system("zip -r $filename addons");
}

#my ($major,$minor) = ($version =~ /^(\d+)\.(\d+)/);
#$ftp_path .= "/$major.$minor";

my ($ftp);

$ftp = Net::FTP->new($ftp_host, Debug => 0) 
    or die "Cannot connect to host $ftp_host : $@";

$ftp->login($ftp_user, $ftp_pass)
    or die "Cannot connect to host $ftp_host as $ftp_user : " . $ftp->message . "\n";

if ($ftp_path ne '')
{
    $ftp->cwd($ftp_path)
        or die "Cannot change to folder $ftp_path : " . $ftp->message . "\n";
}

$ftp->binary();
$ftp->put($filename)
    or die "Cannot drop file $filename ($ftp_path) : " . $ftp->message . "\n";

$ftp->close();

print "File sent to drop site as $filename -- build succeeded.\n";

exit(0);

