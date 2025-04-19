#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

make

mkdir -p dist
cp -a venus/gemini.app venus/gemini.rsc venus/geminiic.rsc venus/gemini.msg dist
cp -a mupfel/mupfel.ttp mupfel/mupfel.msg dist
cp -a runner2/runner2.app dist
