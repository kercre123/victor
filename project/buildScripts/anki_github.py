#!/usr/bin/env python2

#############################################
#                                           #
# Anki_Github                               #
# Library for Anki Specific Github funcions #
#                                           #
#############################################

from github import Github, GithubException
import subprocess
import dependencies
import sys
import os
import requests
from requests.auth import HTTPBasicAuth


DEFAULT_REPO = "anki/cozmo-one"


def get_branch_name(auth_token, pull_request_number, repo_name="anki/cozmo-one"):
    """
    Returns the original developer branch name given a github PR number.

    :param auth_token: string
    :param pull_request_number: int
    :param repo_name: string
    :return: string
    """

    # TO-DO: initialize of instance should become its own function if this grows larger.
    try:
        github_inst = Github(auth_token)
        repo = github_inst.get_repo(repo_name)
        for pr in repo.get_pulls():
            if pr.number == pull_request_number:
                return (pr.raw_data['head']['ref'])
    except GithubException as e:
        print("Exception thrown because of {0}".format(repo))
        sys.exit("GITHUB ERROR: {0}".format(e.data['message']))


def get_file_change_list(auth_token, pull_request_number, verbose=False, repo_name=DEFAULT_REPO):
    """
    Returns the list of changed files from a github PR.

    :param auth_token: string
    :param pull_request_number: int
    :param verbose: boolean
    :param repo_name: string
    :return: list
    """

    if dependencies.is_tool("git") is False:
        sys.exit("[ERROR] Could not find git")

    try:
        github_inst = Github(auth_token)
        repo = github_inst.get_repo(repo_name)
        pr = repo.get_pull(pull_request_number)
    except GithubException as e:
        sys.exit("GITHUB ERROR: {0} {1}".format(e.data['message'], e.status))

    if verbose:
        print("Comparing {0} at {1}".format(pr.base.ref, pr.base.sha))
        print("against {0} at {1}".format(pr.head.ref, pr.head.sha))

    git_cmd = ['git', 'diff', '--name-only', '{0}...{1}'.format(pr.base.sha, pr.head.sha), '--submodule']
    p = subprocess.Popen(git_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    status = p.poll()

    if status != 0:
        print("ERROR: {0}".format(stderr))
        return None
    if verbose:
        print(os.linesep + "Files Changed:" + os.linesep)
        print(stdout)
        return stdout.splitlines()


def pull_request_merge_info(auth_token, pull_request_number, retries=5, verbose=False, repo_name=DEFAULT_REPO):
    """
    Returns a Dictionary of if it's mergable, and the merged sha to build against.

    :param auth_token: string
    :param pull_request_number: int
    :param verbose: boolean
    :param repo_name: string
    :return: Dictionary
    """

    results = {}
    if dependencies.is_tool("git") is False:
        sys.exit("[ERROR] Could not find git")
    for x in range(retries):
        try:
            github_inst = Github(auth_token)
            repo = github_inst.get_repo(repo_name)
            pr = repo.get_pull(pull_request_number)
            results = {"mergeable": pr.mergeable, "merge_commit_sha": pr.merge_commit_sha}
            if pr.mergeable:  # Not sure why some times it takes a few tries.
                break
        except GithubException as e:
            sys.exit("GITHUB ERROR: {0} {1}".format(e.data['message'], e.status))

    if verbose:
        print("Comparing {0} at {1}".format(pr.base.ref, pr.base.sha))
        print("against {0} at {1}".format(pr.head.ref, pr.head.sha))

    return results


class GitRefCollect(object):
    def __init__(self):
        try:
            self.auth_token = os.environ['ANKI_AUTH_TOKEN']
        except KeyError:
            sys.exit("Please set the environment variable [ANKI_AUTH_TOKEN]")

        self.repos = ["cozmo-one", "victor"]  # can be changed to include more repos
        self.tags_list = ["no-release/final-1.5.1", "release-1.0.0",
                          "release/candidate-1.1.0.156", "release/candidate-1.1.0.157",
                          "release/final-1.1.0", "release/final-1.1.1",
                          "release/final-1.3.0", "release/final-1.4.0",
                          "release/final-1.4.1", "release/final-1.5.0",
                          "release/final-1.6.0-android", "release/final-1.6.0",
                          "release/final-1.7.1", "release/final-2.0.0",
                          "release/final-2.0.1", "release/final-2.0.2",
                          "release/final-2.0.3", "release/final-2.0.4",
                          "release/final-2.1.0", "release/final-2.2.0",
                          "release/final-2.3.0", "release/final-2.3.1",
                          "release/final-2.4.0", "release/final-2.5.0",
                          "release/final-2.6.0", "release/final-2.6.1",
                          "release/final-2.7.0", "release/final-2.8.0",
                          "release/final-2.9.0", "release/patch-it-1.0.1.115",
                          "release/patch-it-1.0.2.136", "release/patch-it-1.0.3.140",
                          "release/ship-it-merge-01", "release/ship-it-origin"]

    def collect_old_branches(self, repo_name):
        """
        Returns a dictionary of branches with pr number as key, and branch name as value
        This does the same logic as above get_branch_name, but without the need for the PR number as a param
        :return: Dictionary
        """
        try:
            pull_requests = requests.get(
                "https://api.github.com/repos/anki/{0}/pulls?state=closed&base=master".format(repo_name),
                auth=HTTPBasicAuth("ankibuildserver", gitrefobj.auth_token)).json()
            git_reference = requests.get("https://api.github.com/repos/anki/{0}/git/refs/heads".format(repo_name),
                                         auth=HTTPBasicAuth("ankibuildserver", gitrefobj.auth_token)).json()
            del_branches = {}
            refs_dict = {}
            for pull_request in pull_requests:
                pr_num = pull_request['number']
                pr_branch_ref = "refs/heads/%s" % pull_request['head']['ref']
                pr_merged_data = requests.get(
                    "https://api.github.com/repos/anki/{0}/pulls/{1}".format(repo_name, pr_num),
                    auth=HTTPBasicAuth("ankibuildserver", gitrefobj.auth_token)).json()
                pr_is_merged = pr_merged_data['merged']  # boolean
                if pr_is_merged is True:
                    del_branches[pr_branch_ref] = pr_num
            for reference in git_reference:
                ref_name = reference['ref']
                for item_name, item_num in del_branches.items():
                    if ref_name == item_name:
                        refs_dict[ref_name] = item_num
            return refs_dict
        except Exception as e:
            sys.exit("GITHUB ERROR: {0}".format(e))

    def collect_old_tags(self, tag_list, repo_name='victor'):
        """
        Returns a dictionary of branches with pr number as key, and branch name as value
        This does the same logic as above get_branch_name, but without the need for the PR number as a param
        :return: Dictionary
        """
        try:
            git_reference = requests.get("https://api.github.com/repos/anki/{0}/git/refs/tags".format(repo_name),
                                         auth=HTTPBasicAuth("ankibuildserver", gitrefobj.auth_token)).json()
            tags_to_delete = ["{0}/{1}".format("refs/tags", tag_name) for tag_name in tag_list]
            tags_ref_list = []
            for reference in git_reference:
                ref_tag_name = reference['ref']
                for item_name in tags_to_delete:
                    if ref_tag_name == item_name:
                        tags_ref_list.append(ref_tag_name)
            return tags_ref_list
        except Exception as e:
            sys.exit("GITHUB ERROR: {0}".format(e))

    def delete_tag(self, repo_name, tag_ref_name):
        return requests.delete("https://api.github.com/repos/anki/{0}/git/{1}".format(repo_name, tag_ref_name),
                               auth=HTTPBasicAuth("ankibuildserver", gitrefobj.auth_token))

    def delete_branch(self, repo_name, branch_ref):
        return requests.delete("https://api.github.com/repos/anki/{0}/git/{1}".format(repo_name, branch_ref),
                               auth=HTTPBasicAuth("ankibuildserver", gitrefobj.auth_token))


if __name__ == '__main__':
    dry_run = '--confirm' not in sys.argv
    gitrefobj = GitRefCollect()
    for repo in gitrefobj.repos:
        if dry_run:
            print("Performing Dry-run. Currently checking <{0}> repo for closed, merged but not deleted branches.\n{1}."
                  .format(repo, gitrefobj.collect_old_branches(repo)))
            if '--tags' in sys.argv:
                print("Currently checking <{0}> repo.\nFound old tags: {1}."
                      .format(repo, gitrefobj.collect_old_tags(gitrefobj.tags_list, repo)))
        else:
            print("Currently checking <{0}> repo for closed, merged but not deleted branches.".format(repo))
            for branch in gitrefobj.collect_old_branches(repo):
                print(gitrefobj.delete_branch(repo, branch))
            print("Currently checking <{0}> repo. Deleting old tags: {1}"
                  .format(repo, gitrefobj.collect_old_tags(gitrefobj.tags_list, repo)))
            for tag in gitrefobj.collect_old_tags(gitrefobj.tags_list, repo):
                print(gitrefobj.delete_tag(repo, tag))
    if dry_run:
        print('*****************************************************************')
        print('Did not actually delete anything yet, pass in --confirm to delete')
        print('*****************************************************************')
