# Notes to Self

To create a new snapshot, for better or worse:

* Create the release using normal JLSS mechanism.
* Extract scc-x.yz.tgz into scc-x.yz parallel to this directory.
* Copy changed files (most source files because they're version stamped;
  often not the data files) into the scc-snapshot repository.
  For example:

        cd scc-x.yz
        cp -fpr . ../scc-snapshots

* Update the README.md file with the new release information.
* Add the changed files.
* Remove the old checksum files.
* Add the new checksum files.
* Commit the changes.
* Push the changes to GitHub.
* Create a release on GitHub.

