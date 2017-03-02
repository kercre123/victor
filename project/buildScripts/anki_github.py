#!/usr/bin/env python

#############################################
#                                           #
# Library for Anki Specific Github funcions #
#                                           #
#############################################

from github import Github


def get_branch_name(auth_token, pull_request_number, repo_name="anki/cozmo-one"):
    """
    Returns the original developer branch name given a github PR number.

    :param auth_token: string
    :param pull_request_number: int
    :param repo_name: string
    :return: string
    """

    # TO-DO: initialize of instance should become its own function if this grows larger.
    github_inst = Github(auth_token)
    repo = github_inst.get_repo(repo_name)
    try:
        for pr in repo.get_pulls():
            if pr.number == pull_request_number:
                return(pr.raw_data['head']['ref'])
    except:
	print("Exception thrown because of {0}".format(repo))
