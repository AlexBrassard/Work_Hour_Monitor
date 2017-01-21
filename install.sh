#!/bin/bash

########################################
#
# Work Hour Monitor installation script.
#
########################################

INSTALL_DIR="/usr/local/bin"
PERM=0710                    # rwx--x--x
PROGNAME="whm"

function create_sysdep_header() {
    printf "\nWork Hour Monitor installation script.\n\n"
    printf "Please input the absolute pathname where to create the .whm.d/ directory: "
    read WORKING_DIRECTORY
    
    if [ ! -d "$WORKING_DIRECTORY/.whm.d" ]
    then
	mkdir "$WORKING_DIRECTORY/.whm.d"
    fi
    
    printf "/*\n *\n * Work Hour Monitor  -  System dependent values.\n *\n */\n\n" > ./whm_sysdep.h
    printf "#ifndef WHM_SYSDEP_HEADER_FILE\n# define WHM_SYSDEP_HEADER_FILE\n" >> ./whm_sysdep.h
    printf "\n/* Directory in which Whm saves and fetches informations. */\n" >> ./whm_sysdep.h
    printf "static const char WHM_WORKING_DIRECTORY[] = \"%s/.whm.d\";\n\n" $WORKING_DIRECTORY >> ./whm_sysdep.h
    printf "/* Absolute pathname of the configuration file. */\n" >> ./whm_sysdep.h
    printf "static const char WHM_CONFIGURATION_FILE[] = \"%s/.whm.d/.whm_config\";\n\n" $WORKING_DIRECTORY >> ./whm_sysdep.h
    printf "\n\n#endif /* WHM_SYSDEP_HEADER_FILE */" >> ./whm_sysdep.h
}

# Compile the binary and move it in the appropriate directory. 

if [ ! -f "$PWD/whm_sysdep.h" ]
then
    create_sysdep_header
fi

make clean

make

if [ $? != 0 ]
then
    exit
fi

chmod $PERM $PROGNAME

sudo mv $PROGNAME $INSTALL_DIR
