#!/usr/bin/perl
#
# Copyright (c) 2001
#             Tama Communications Corporation. All rights reserved.
#
# This file is part of GNU GLOBAL.
#
# GNU GLOBAL is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# GNU GLOBAL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# Main procedure.
#
$com = $0;
$com =~ s/.*\///;
while ($ARGV[0] =~ /^-/) {
        $opt = shift;
        if ($opt eq '--c') {
                $c = 1;
        } elsif ($opt eq '--perl') {
                $perl = 1;
        } elsif ($opt eq '--man') {
                $man = 1;
	} elsif ($opt eq '--info') {
                $info = 1;
	}
}
$infile = $ARGV[0];
open(INFILE, $infile) || die("cannot open '$infile'\n");
if ($c) {
	&c'gen();
} elsif ($perl) {
	&perl'gen();
} elsif ($man) {
	&man'gen();
} elsif ($info) {
	&info'gen();
}
close(INFILE);
exit 0;
#------------------------------------------------------------------
#
# Read line.
#
#------------------------------------------------------------------
$lastline = '';
sub getline {
	if ($lastline) {
		$_ = $lastline;
		$lastline = '';
	} else {
		$_ = <INFILE>;
	}
	($_) ? 1 : 0;
}
sub ungetline {
	$lastline = $_;
}
#------------------------------------------------------------------
#
# C package.
#
#------------------------------------------------------------------
package c;
sub convert {
	local($arg) = '[^},]+';
	local($macros) = 'arg|code|var|file|name|option';
	chop;
	s/^\s+//;
	while (s/\@($macros)\{($arg)\}/$2/) {
		;
	}
	while (s/\@xref\{($arg),($arg)\}/$1($2)/) {
		;
	}
}
sub gen {
	local($help_const) = 0;
	print "/* This file is generated automatically by $'com from $'infile. */\n";
	while (&'getline()) {
		if (/^\@NAME\s+(.*)\s+-/) {
			print "const char *progname = \"$1\";\n";
		} elsif (/^\@SYNOPSIS$/) {
			local($first) = 1;
			print "const char *usage_const = \"";
			while (&'getline()) {
				if (/^\@/) {
					&'ungetline();
					last;
				}
				convert();
				if ($first) {
					$first = 0;
					print "Usage: ";
				} else {
					print "\\n\\\n";
					print "       ";
				}
				print $_;
			}
			print "\\n\";\n";
		} elsif (/^\@COMMANDS$/ || /^\@OPTIONS$/) {
			if (!$help_const) {
				$help_const = 1;
				print "const char *help_const = \"";
			}
			if (/^\@COMMANDS$/) {
				print "Commands:";
			} else {
				print "Options:";
			}
			print "\\n\\\n";
			local($itemize) = 0;
			while (&'getline()) {
				if (/^\@/) {
					&'ungetline();
					last;
				}
				convert();
				if (/^\@begin_itemize$/) {
					$itemize = 1;
				} elsif (/^\@end_itemize$/) {
					$itemize = 0;
				} elsif ($itemize) {
					if (/^\@item\{(.*)\}$/) {
						print $1;
					} else {
						print "       ";
						print;
					}
					print "\\n\\\n";
				}
			}
		}
	}
	print "\";\n";
}
#------------------------------------------------------------------
#
# Perl package.
#
#------------------------------------------------------------------
package perl;
sub convert {
	local($arg) = '[^},]+';
	while (s/\@file\{($arg)\}/'$1'/) {
		;
	}
	&c'convert();
}
sub gen {
	local($help_const) = 0;
	print "# This part is generated automatically by $'com from $'infile.\n";
	while (&'getline()) {
		if (/^\@NAME\s+(.*)\s+-/) {
			print "\$program = '$1';\n";
		} elsif (/^\@SYNOPSIS$/) {
			&'getline();
			convert();
			print "\$usage_const = \"Usage: $_\";\n";
		} elsif (/^\@OPTIONS$/) {
			print "\$help_const = \"\$usage_const\\\nOptions:\\\n";
			while (&'getline()) {
				if (/^\@/) {
					&'ungetline();
					last;
				}
				convert();
				if (/^\@begin_itemize$/) {
					$itemize = 1;
				} elsif (/^\@end_itemize$/) {
					$itemize = 0;
				} elsif ($itemize) {
					if (/^\@item\{(.*)\}$/) {
						print $1;
					} else {
						print "       ";
						print;
					}
					print "\\\n";
				}
			}
		}
	}
	print "\";\n";
	print "# end of generated part.\n";
}
#------------------------------------------------------------------
#
# Man package.
#
#------------------------------------------------------------------
package man;
sub convert {
	local($arg) = '[^},]+';
	s/^\s+//;
	while (s/\@arg\{($arg)\}/\\fI$1\\fP/) {
		;
	}
	while (s/\@file\{($arg)\}/'$1'/) {
		;
	}
	while (/\@code\{($arg)\}/) {
		$val = $1;
		$val =~ s/\./\\./g;
		s/\@code\{$arg\}/$val/;
	}
	while (s/\@(code|var|name|option)\{($arg)\}/\\fB$2\\fP/) {
		;
	}
	while (s/\@xref\{($arg),($arg)\}/\\fB$1\\fP($2)/) {
		;
	}
}
sub gen {
	local($arg) = '[^,]+';
	print ".\\\" This file is generated automatically by $'com from $'infile.\n";
	while (&'getline()) {
		next if (/^#/);		# comment
		if (/^\@HEADER\s+($arg),($arg),($arg),($arg)\n$/) {
			print ".TH $1 $2 \"$3\" \"$4\"\n";
		} elsif (/^\@(NAME)\s+(.*)$/) {
			local($name) = $2;
			$name =~ s/-/\\-/;
			print ".SH $1\n$name\n";
		} elsif (/^\@(SYNOPSIS)$/) {
			print ".SH $1\n";
			while (&'getline()) {
				if (/^\@/) {
					&'ungetline();
					last;
				} elsif (/^$/) {
					print ".PP\n";
					next;
				}
				convert();
				print;
				print ".br\n";
			}
		} elsif (/^\@(.*)$/) {
			$type = $1;
			local($varbatim) = ($type =~ /^(EXAMPLES|FORMAT)$/) ? 1 : 0;
			if ($type =~ /\s+/) {
				$type = "\"$type\"";
			}
			print ".SH $type\n";
			print ".nf\n" if ($varbatim);
			local($itemize) = 0;
			while (&'getline()) {
				if (/^\@/) {
					&'ungetline();
					last;
				} elsif (/^$/) {
					print ".PP\n";
					next;
				}
				convert();
				if (/^\@begin_itemize$/) {
					$itemize = 1;
				} elsif (/^\@end_itemize$/) {
					$itemize = 0;
				} elsif ($itemize && /^\@item\{(.*)\}$/) {
					print ".TP\n$1\n";
				} else {
					print;
				}
			}
			print ".fi\n" if ($varbatim);
		}
	}
}
#------------------------------------------------------------------
#
# Info package.
#
#------------------------------------------------------------------
package info;
sub convert {
	local($arg) = '[^},]+';
	chop;
	s/^\s+//;
	while (s/\@xref\{($arg),($arg)\}/$1($2)/) {
		;
	}
	s/\@option\{/\@samp\{/g;
	while (s/\@(arg|name)\{($arg)\}/$2/) {
		;
	}
}
sub gen {
	print "\@c This file is generated automatically by $'com from $'infile.\n";
	while (&'getline()) {
		next if (/^#/);		# comment
		if (/^\@HEADER/) {
			;
		} elsif (/^\@(NAME)\s+(.*)$/) {
			local($name) = $2;
			print "\@unnumberedsubsec $1\n";
			print "$name\n";
		} elsif (/^\@(SYNOPSIS)$/) {
			print "\@unnumberedsubsec $1\n";
			print "\@noindent\n";
			print "\@quotation\n";
			while (&'getline()) {
				if (/^\@/) {
					&'ungetline();
					last;
				} elsif (/^$/) {
					print;
					next;
				}
				convert();
				print;
				print "\@*\n";
			}
			print "\@end quotation\n";
		} elsif (/^\@(.*)$/) {
			$type = $1;
			local($varbatim) = ($type =~ /^(EXAMPLES|FORMAT)$/) ? 1 : 0;
			if ($type =~ /\s+/) {
				$type = "\"$type\"";
			}
			print "\@unnumberedsubsec $type\n";
			print "\@example\n" if ($varbatim);
			local($itemize) = 0;
			while (&'getline()) {
				if (/^\@/) {
					&'ungetline();
					last;
				} elsif (/^$/) {
					print;
					next;
				}
				convert();
				if (/^\@begin_itemize$/) {
					$itemize = 1;
					print "\@table \@asis\n";
				} elsif (/^\@end_itemize$/) {
					$itemize = 0;
					print "\@end table\n";
				} elsif ($itemize && /^\@item\{(.*)\}$/) {
					print "\@item $1\n";
				} else {
					print "$_\n";
				}
			}
			print "\@end example\n" if ($varbatim);
		}
	}
}
