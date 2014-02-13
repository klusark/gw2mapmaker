#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

$DIR/extract.py $DIR/out.json
$DIR/split.py $dir/layers.json
$DIR/squish.py
$DIR/combine.py
