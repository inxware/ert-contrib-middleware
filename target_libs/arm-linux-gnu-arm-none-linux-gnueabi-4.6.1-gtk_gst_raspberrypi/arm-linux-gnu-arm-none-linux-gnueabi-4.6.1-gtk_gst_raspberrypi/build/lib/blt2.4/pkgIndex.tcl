# Tcl package index file, version 1.0

proc LoadBLT { version dir } {

    set prefix "lib"
    set suffix [info sharedlibextension]
    regsub {\.} $version {} version_no_dots

    # Determine whether to load the full BLT library or
    # the "lite" tcl-only version.

    if {[package vcompare [info tclversion] 8.2] < 0} {
        set taillib ${version}.so.8.0
    } elseif {[package vcompare [info tclversion] 8.3] < 0} {
        set taillib ${version}.so.8.2
    } elseif {[package vcompare [info tclversion] 8.4] < 0} {
        set taillib ${version}.so.8.3
    } elseif {[package vcompare [info tclversion] 8.5] < 0} {
        set taillib ${version}.so.8.4
    } else {
        set taillib ${version}.so.8.5
    }

    if { [info commands tk] == "tk" } {
        set name ${prefix}BLT.${taillib}
    } else {
        set name ${prefix}BLTlite.${taillib}
    }

    global tcl_platform
    if { $tcl_platform(platform) == "unix" } {
        if { [info commands tk] == "tk" } {
          set library libBLT.${taillib}
        } else {
          set library libBLTlite.${taillib}
        }
	if { ![file exists $library] } {
	    # Try the parent directory.
	    set library [file join [file dirname $dir] $name]
	}
	if { ![file exists $library] } {
	    # Default to the path generated at compilation.
	    set library [file join "/usr/lib" $name]
	}
    } else {
	set library $name
    }
    load $library BLT
}

set version "2.4"
set libdir  "/usr/lib"

package ifneeded BLT $version [list LoadBLT $version $dir]

# End of package index file
