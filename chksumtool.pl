#!/usr/bin/env perl
#
#   @(#)$Id: chksumtool.pl,v 1.14 2015/10/18 16:43:22 jleffler Exp $
#
#   Checksum Tool for JLSS Distribution Software

# Command line options:
#    -a alg         Which checksum algorithm (default: SHA256)
#    -c             Create checksum files
#    -v             Verify checksum files
#    -q             Quiet mode (during verify)
#    -l file.list   Files list (eg a .nmd file)
#    -d details.alg Detailed checksums for each file
#    -s summary.alg Summary checksum for detail file
#
#   * You must specify either the -c or the -v options; the rest are
#     optional given an appropriate context.
# On create:
#   * You must either specify -l or some files as command line
#     arguments, or the list of files on standard input.
#   * If you do not specify -d, the details go to stdout.
#   * If you do not specify -s, no summary is generated.
# On verify:
#   * If you specify -d, then you do not need to specify any file list.
#   * If you specify -s, the file should contain one checksum citing the
#     detail file (and you do not have to specify -d).
#   * If you specify -l, the code can validate the file list against the
#     detail list.
# NB: Although the input could have different checksum algorithms per
# line, the file would not have been generated by this tool (unless
# separate runs were concatenated).

use warnings;
use strict;
use Getopt::Std;
use Digest;
use File::Basename;
use FileHandle;

# Constant values for stat/lstat elements (there should be a standard package for this!)
use constant ST_DEV     =>  0;  #  0  dev       device number of filesystem
use constant ST_INO     =>  1;  #  1  ino       inode number
use constant ST_MODE    =>  2;  #  2  mode      file mode  (type and permissions)
use constant ST_NLINK   =>  3;  #  3  nlink     number of (hard) links to the file
use constant ST_UID     =>  4;  #  4  uid       numeric user ID of file's owner
use constant ST_GID     =>  5;  #  5  gid       numeric group ID of file's owner
use constant ST_RDEV    =>  6;  #  6  rdev      the device identifier (special files only)
use constant ST_SIZE    =>  7;  #  7  size      total size of file, in bytes
use constant ST_ATIME   =>  8;  #  8  atime     last access time in seconds since the epoch
use constant ST_MTIME   =>  9;  #  9  mtime     last modify time in seconds since the epoch
use constant ST_CTIME   => 10;  # 10  ctime     inode change time in seconds since the epoch (*)
use constant ST_BLKSIZE => 11;  # 11  blksize   preferred block size for file system I/O
use constant ST_BLOCKS  => 12;  # 12  blocks    actual number of blocks allocated

my %opts;
my $usestr =
"Usage:  $0 [-hqV] {-c|-v} [-d detailed.sums [-s summary.sums]] [-l file.list | file ...]
Create: $0 -c      [-d detailed.sums [-s summary.sums]] {-l file.list | file ...}
Verify: $0 -v [-q] [-d detailed.sums [-s summary.sums]]
";

die "$usestr\n" unless getopts('Vchqva:d:l:s:', \%opts);

# Version and help first.
if (defined $opts{V})
{
    printf "%s\n", &version;
    exit 0;
}
elsif (defined $opts{h})
{
print "$usestr\n
Command line options:
   -a alg         Which checksum algorithm
   -c             Create checksum files
   -v             Verify checksum files
   -q             Quiet mode
   -l file.file   Files list (eg a .nmd file)
   -d details.alg Detailed checksums for each file
   -s summary.alg Summary checksum for detail file
";
}

# Must specify create or verify but not both
if (defined $opts{c} && defined $opts{v})
{
    print STDERR "$0: cannot simultaneously create (-c) and verify (-v) checksums\n";
    exit 1;
}
elsif (!defined $opts{c} && !defined $opts{v})
{
    print STDERR "$0: must specify either create (-c) or verify (-v) checksums\n";
    exit 1;
}

my $quiet = defined($opts{q}) ? 1 : 0;
my $hash  = defined($opts{a}) ? $opts{a} : "SHA256";
my $chksumfmt = "%s %s %8d %s\n";

$hash =~ tr/[a-z]/[A-Z]/;
$hash =~ s/\d\d\d$/-$&/;

if (defined $opts{c})
{
    create_chksums($opts{l}, $opts{d}, $opts{s});
}
else
{
    verify_chksums($opts{l}, $opts{d}, $opts{s});
}

sub verify_chksums
{
    my($filelist, $details, $summary) = @_;
    # Verify check sums
    # Details and summary files will be read

    # Generate the list of files for which checksums should be generated
    my(@files) = ();
    if (defined $filelist)
    {
        &error("$0: omit the superfluous arguments: @ARGV\n") if (scalar(@ARGV) > 0);
        @files = &read_file_list($filelist);
    }
    elsif (scalar(@ARGV) > 0)
    {
        @files = @ARGV;
    }
    elsif (!defined $details && !defined $summary)
    {
        my($handle) = FileHandle->new_from_fd(fileno(STDIN), "r");
        die "$0: failed to open standard input - $!" unless defined $handle;
        @files = &file_list_from_handle($handle);
    }

    # Read the official version of the checksum of the checksums
    if (defined $summary)
    {
        my(@summary) = &slurp_chksums($summary);
        &error("$0: unexpected data (too many lines) in summary file $summary\n")
            if (scalar(@summary) > 1);
        my ($alg, $sum, $size, $file) = split /\s+/, $summary[0];
        &error("$0: detail file $details is not named in summary file $summary\n")
            if (defined $details && $file ne $details);
        &error("$0: check sum for details file $file is wrong - no point in continuing!\n")
            if (&chksum_for_file($file, $alg, $sum, $size) != 0);
        $details = $file unless defined $details;
    }
    &error("$0: must specify details file with -d option or -s option\n") if !defined($details);

    # Read the official version of the checksums
    # KLUDGE: no support for checksums from stdin!
    my(@chksums) = &slurp_chksums($details);

    # Convert the array of "checksum file" entries into a hash of the
    # checksum values keyed by file name.
    my(%chksum);
    foreach (@chksums)
    {
        my ($alg, $sum, $size, $file) = split;
        $chksum{$file} = [ $alg, $sum, $size ];
    }

    # @files contains the list of files that should be validated.
    # %chksum contains the official checksum indexed by file name; it is
    # assumed that there are no spaces in the file names.
    # We need to generate the SHA256 checksums for those files, and then
    # compare what we calculate with what is in %chksum hash.

    my($rc) = 0;

    foreach my $file (sort keys %chksum)
    {
        my $ref= $chksum{$file};
        $rc |= &chksum_for_file($file, @{$chksum{$file}});
    }

    foreach my $file (sort @files)
    {
        if (!defined $chksum{$file})
        {
            &warning("List of files did not mention $file\n");
            $rc = 1;
        }
    }

    print(($rc == 0) ? "OK\n" : "*** FAILED ***\n");

    exit $rc;
}

sub write_chksums
{
    my($dfile, $alg, @files) = @_;
    foreach my $file (sort @files)
    {
        my ($size) = &file_size($file);
        my ($algstr, $result) = &generate_digest_one_file($alg, $file);
        $dfile->print(sprintf($chksumfmt, $algstr, $result, $size, $file));
    }
}

# Create check sums - write details and summary files
sub create_chksums
{
    my($filelist, $details, $summary) = @_;

    # Generate the list of files for which checksums should be generated
    my(@files) = ();
    if (defined $filelist)
    {
        &error("$0: omit the superfluous arguments: @ARGV\n") if (scalar(@ARGV) > 0);
        @files = &read_file_list($filelist);
    }
    elsif (scalar(@ARGV) > 0)
    {
        @files = @ARGV;
    }
    else
    {
        my($handle) = FileHandle->new_from_fd(fileno(STDIN), "r");
        die "$0: failed to open standard input - $!" unless defined $handle;
        @files = &file_list_from_handle($handle);
    }

    if (defined $details)
    {
        my($dfile) = &open_file(">$details");
        write_chksums($dfile, $hash, @files);
        $dfile->close if defined $details;
        if (defined $summary)
        {
            my ($sfile) = &open_file(">$summary");
            my ($size) = &file_size($details);
            my ($algstr, $result) = &generate_digest_one_file($hash, $details);
            my ($string) = sprintf("%s %-32s %-6d %s\n", $algstr, $result, $size, $details);
            $sfile->print($string);
            $sfile->close;
            print $string unless $quiet;
        }
    }
    else
    {
        my($dfile) = FileHandle->new_from_fd(fileno(STDOUT), "w");
        die "$0: failed to open standard output - $!" unless defined $dfile;
        die "WTF! $!" unless defined $dfile;
        write_chksums($dfile, $hash, @files);
    }
}

sub file_size
{
    my($file) = @_;
    my(@stat) = lstat $file or die "lstat($file) failed: $!";
    return($stat[ST_SIZE]);
}

sub version
{
    my $vrsn = '$Revision: 1.14 $ ($Date: 2015/10/18 16:43:22 $)';
    $vrsn =~ s/\$([A-Z][a-z]+): ([^\$]+) \$/$2/go;
    my($prog) = fileparse($0, qr/\.pl$/);
    $prog =~ tr/a-z/A-Z/;
    return "$prog version $vrsn";
}

sub generate_digest_one_file
{
    my($alg, $file) = @_;
    my $digest = "";
    my $algstr = $alg;
    if (! -l $file && -f $file && -r $file)
    {
        my($ctx) = eval { Digest->new($alg); };
        die "$0: failed to create Digest context for $alg\n$@\n" if ($@);
        my($handle) = &open_file("<$file");
        binmode($handle);
        $ctx->addfile($handle);
        $digest = $ctx->hexdigest;
        $handle->close;
    }
    else
    {
        # Note: alternatives to checksums contain no spaces!
        # Note: the '=' symbol is to make it easy to identify non-checksums.
        my $link = readlink $file if -l $file;
        $digest = (! -e $file && -l $file) ? "broken-symlink-to:$link"
                : (! -e $file)             ? "non-existent"
                :   (-l $file)             ? "symlink-to:$link"
                :   (-S $file)             ? "socket"
                :   (-c $file)             ? "character-special"
                :   (-b $file)             ? "block-special"
                :   (-d $file)             ? "directory"
                :   (-p $file)             ? "fifo"
                : (! -r $file && -f $file) ? "unreadable"
                : "unknown-file-type"
                ;
        $digest = "$digest";
        $algstr =~ s/./-/go;
    }
    return ($algstr, $digest);
}

sub file_list_from_handle
{
    my($handle) = @_;
    my(@data) = ();
    while (<$handle>)
    {
        chomp;
        s/#.*//o;           # Remove comments
        next if m/^\s*$/o;  # Ignore blank lines
        s/\s+.*//o;         # Remove CM file and version information
        push @data, $_;
    }
    die "$0: no file names to process\n" if (scalar(@data) <= 0);
    return(@data);
}

sub open_file
{
    my($name) = @_;
    my($file) = new FileHandle "$name";
    die "$0: failed to open '$name' - $!" unless defined $file;
    return($file);
}

sub read_file_list
{
    my($name) = @_;

    if (-s $name)
    {
        my($file) = &open_file("<$name");
        my(@data) = file_list_from_handle($file);
        $file->close;
        return(@data);
    }

    die "$name is empty\n" if (-f $name);
    die "$name does not exist or is not a regular file\n";
}

sub slurp_chksums
{
    my($name) = @_;

    if (-s $name)
    {
        my($file) = &open_file("<$name");
        my(@data) = ();
        while (<$file>)
        {
            chomp;
            push @data, $_;
        }
        close $file;
        die "$0: no file names to process in $name\n" if (scalar(@data) <= 0);
        return(@data);
    }

    die "$name is empty\n" if (-f $name);
    die "$name does not exist or is not a regular file\n";
}

sub chksum_for_file
{
    my($file, $alg, $chksum, $chksize) = @_;
    my($rc) = 0;

    my($actsize) = &file_size($file);
    if ($actsize != $chksize)
    {
        &warning("*** ERROR: file sizes differ for $file\n");
        &warning("    Official value: $chksize\n");
        &warning("    Derived  value: $actsize\n");
        $rc = 1;
    }
    else
    {
        my($algstr, $actsum) = &generate_digest_one_file($alg, $file);
        printf $chksumfmt, $algstr, $actsum, $actsize, $file unless $quiet;
        if ($actsum ne $chksum)
        {
            &warning("*** ERROR: $alg checksums differ for $file\n");
            &warning("    Official value: $chksum\n");
            &warning("    Derived  value: $actsum\n");
            $rc = 1;
        }
    }
    return($rc);
}

sub warning
{
    my($message) = @_;
    print STDERR $message;
}

sub error
{
    my($message) = @_;
    print STDERR $message;
    exit 1;
}

=pod

=head1 NAME

chksumtool - A Tool for Generating and Validating Checksums

=head1 SYNOPSIS

=head1 DESCRIPTION

=head1 LICENCE

This program is distributed under the same terms as Perl.

=head1 AUTHOR

Jonathan Leffler M<jonathan.leffler@gmail.com>

=cut
