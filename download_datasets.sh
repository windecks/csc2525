#!/bin/bash

mkdir -p datasets
cd datasets

echo "Downloading Silesia corpus..."
wget -q -nc https://sun.aei.polsl.pl/~sdeor/corpus/silesia.zip
unzip -q -o silesia.zip -d silesia

echo "Downloading enwiki8..."
wget -q -nc http://mattmahoney.net/dc/enwiki8.zip
unzip -q -o enwiki8.zip

echo "Datasets downloaded and extracted."
