#!/bin/bash

# This script will be used only for updating externals to the newest version.
# I've decided to have externals in my own repository (not links)
# Because they will be patched.
# After fetching externals, rsync will create "(filename)~" files, with
# patched (old) versions.


DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
mkdir -p $DIR/clones

function updateOrFetchGit {
    mainExternalRepoName=$1
    externalRepo=$2
    externalName=$3
    externalSubdir=$4

    if [ -d "$DIR/clones/$mainExternalRepoName/.git" ]
    then
        echo "$mainExternalRepoName exists, just update"
        cd $DIR/clones/$mainExternalRepoName && git pull
    else
        echo "Fetching $mainExternalRepoName"
        cd $DIR/clones && rm -rf $mainExternalRepoName
        cd $DIR/clones && git clone $externalRepo
    fi

    cd $DIR
    rsync -c -b -h --progress -r $DIR/clones/$mainExternalRepoName/$externalSubdir/* $DIR/$externalName
}

# Not used any more.
#updateOrFetchGit LuminanceHDR https://github.com/LuminanceHDR/LuminanceHDR.git Libpfs src/Libpfs

