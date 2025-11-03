#! /bin/bash
#

cat <<EOF

>>> Pushing our work to both remotes...

EOF
for f in GerHobbelt ; do 
    echo ""
    echo "::REPO: git@github.com:$f/libxml2.git"
    git push --all --follow-tags git@github.com:$f/libxml2.git 		                                        2>&1
    git push --tags              git@github.com:$f/libxml2-orig.git                                         2>&1
done

