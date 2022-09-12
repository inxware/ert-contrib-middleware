#!/usr/bin/perl -n

#case: within a non-C <locale> tag
if ($in_locale) {
    if (/^\s*<default>/) {
	if (!$wrote_locale_tag) {
	    print $locale_tag;
	    $wrote_locale_tag = 1;
	}
	print $_;
    } elsif (/^\s*<\/locale>/) {
	print $_ if $wrote_locale_tag;
	$in_locale = 0;
    }
}
# case: locale name="C": add <gettext_domain> and leave alone
elsif (/^(\s*)<locale name=\"C\">/) {
    print "$1<gettext_domain>$ENV{GETTEXT_DOMAIN}</gettext_domain>\n";
    print $_;
} 
# case: locale name="something": start $in_locale and purge <short> and <long>
elsif (/<locale name=\"[^C]/) {
    $in_locale = 1;
    $wrote_locale_tag = 0;
    $locale_tag = $_;
}
else {
    print $_ unless /^$/;
    print "\n" if /^\s*<\/schema>/; # to not kill all whitespace
}
