#!/bin/bash

mkdir -p datasets
cd datasets

echo "Downloading Silesia corpus..."
wget -q -nc https://sun.aei.polsl.pl/~sdeor/corpus/silesia.zip
unzip -q -o silesia.zip -d silesia

echo "Downloading enwiki8..."
wget -q -nc http://www.mattmahoney.net/dc/enwik8.zip
unzip -q -o enwik8.zip

echo "Datasets downloaded and extracted."
