#!/usr/bin/python3
# -*- coding:utf8 -*-
'''
Gerrit Reviews Watcher
A simple Flask server to fetch all pull request reviews for the specified user
and return the respective colors for a Code Review Lamp to shine.
'''
import json
from flask import Flask, request, make_response
import requests

usernames_to_colors = {"colleague0": "red", "colleague1": "blue",
                       "colleague2": "purple", "colleague3": "yellow", "colleague4": "orange", "colleague5": "green"}
app = Flask("GitHub Review Requests Watcher")


@app.route('/github_reviews/<username>/<oauth_token>', methods=['GET'])
def github_watcher(username, oauth_token):
    github_api_url = "https://api.github.com/"
    search_endpoint = "search/issues?q=is:open+is:pr+review-requested:" + \
        username + "+archived:false"
    request_url = github_api_url + search_endpoint
    header_values = {"Accept": "application/vnd.github.v3+json",
                     "Authorization": "token " + oauth_token,
                     "User-Agent": "Code-Review-Lamp", "Connection": "close"}

    r = requests.get(url=request_url, headers=header_values)
    search_result = r.json()

    colors = str()
    for review_request in search_result["items"]:
        requester = review_request["user"]["login"]
        if requester in usernames_to_colors:
            colors += usernames_to_colors[requester] + ","
        else:
            colors += "white,"

    return colors


def main():
    app.run()


if __name__ == '__main__':
    main()
