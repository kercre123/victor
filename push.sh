#!/bin/bash

# Get the list of all branches, including remote-tracking ones, and loop through them
for branch in $(git branch --all | grep -v '\->' | grep -v 'remotes/' | sed 's/\*//g' | sed 's/^ *//g'); do
    # Checkout the branch
    #echo "Switching to branch $branch"
    #git checkout $branch
    
    # Push the branch
    echo "Pushing branch $branch"
    git push origin $branch
done

# Go back to the branch you were on originally
git checkout -

