#!/bin/bash

xsd cxx-tree --generate-serialization --hxx-suffix .hpp --cxx-suffix .cpp \
    --cxx-prologue "#define COVERAGE_IGNORE
/** @autogenerated */" --cxx-epilogue "#undef COVERAGE_IGNORE" \
    --hxx-prologue "#define COVERAGE_IGNORE
/** @autogenerated */" --hxx-epilogue "#undef COVERAGE_IGNORE" \
    --root-element ChasteParameters ChasteParameters.xsd
