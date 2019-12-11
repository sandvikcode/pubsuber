# If you would like to contribute and submit your own code to pubsuber

## Contributing A Patch

1. Submit an issue describing your proposed change to the repo in question.
1. The repo owner will respond to your issue promptly.
1. Fork the desired repo, develop and test your code changes.
1. Ensure that your code adheres to the existing style in the sample to which
   you are contributing.
1. Ensure that your code has an appropriate set of unit tests which all pass.
1. Submit a pull request.

### More Information on Forks and Pull Requests

If you are just getting started with `git` and [GitHub](https://github.com),
this section might be helpful. In this project we use the (more or less)
standard [GitHub workflow][workflow-link]:

1. You create a [fork][fork-link] of [pubsuber][repo-link]. Then
   [clone][about-clone] that fork into your workstation:
   ```console
   git clone git@github.com:YOUR-USER-NAME/pubsuber.git
   ```
1. The cloned repo that you created in the previous step will have its `origin`
   set to your forked repo. You should now tell git about the the main
   `upstream` repo, which you'll use to pull commits made by others in order to
   keep your local repo and fork up to date.
   ```console
   git remote add upstream git@github.com:sandvikcode/pubsuber.git
   git remote -v  # Should show 'origin' (your fork) and 'upstream' (main repo)
   ```
1. To pull new commits from `upstream` into your local repo and
   [sync your fork][syncing-a-fork] you can do the following:
   ```console
   git checkout develop
   git pull --ff-only upstream develop
   git push  # Pushes new commits up to your fork on GitHub
   ```
   Note: You probably want to do this periodically, and almost certainly before
   starting your next task.
1. You pick an existing [GitHub bug][mastering-issues] to work on.
1. You start a new [branch][about-branches] for each feature (or bug fix).
   ```console
   git checkout develop
   git checkout -b my-feature-branch
   git push -u origin my-feature-branch  # Tells fork on GitHub about new branch
   # make your changes
   git push
   ```
1. You submit a [pull-request][about-pull-requests] to merge your branch into
   `sandvikcode/pubsuber`.
1. Your reviewers may ask questions, suggest improvements or alternatives. You
   address those by either answering the questions in the review or adding more
   [commits][about-commits] to your branch and `git push` -ing those commits to
   your fork.
1. From time to time your pull request may have conflicts with the destination
   branch (likely `develop`), if so, we request that you [rebase][about-rebase]
   your branch instead of merging. The reviews can become very confusing if you
   merge during a pull request. You should first ensure that your `develop`
   branch has all the latest commits by syncing your fork (see above), then do
   the following:
   ```console
   git checkout my-feature-branch
   git rebase develop
   git push --force-with-lease
   ```
1. Eventually the reviewers accept your changes, and they are merged into the
   `develop` branch.

[workflow-link]: https://guides.github.com/introduction/flow/
[fork-link]: https://guides.github.com/activities/forking/
[repo-link]: https://github.com/sandvikcode/pubsuber.git
[mastering-issues]: https://guides.github.com/features/issues/
[about-clone]: https://help.github.com/articles/cloning-a-repository/
[about-branches]: https://help.github.com/articles/about-branches/
[about-pull-requests]: https://help.github.com/articles/about-pull-requests/
[about-commits]: https://help.github.com/desktop/guides/contributing-to-projects/committing-and-reviewing-changes-to-your-project/#about-commits
[about-rebase]: https://help.github.com/articles/about-git-rebase/
[syncing-a-fork]: https://help.github.com/articles/syncing-a-fork/

## Style

This repository contains `.clang-format` file with used style and `format.sh` to format sources.
Please make sure your contributions adhere to the style guide.

## Advanced Compilation and Testing

Please see the [README](README.md) for the basic instructions on how to compile
the code.  In this section we will describe some advanced options.

### Changing the Compiler

If your workstation has multiple compilers (or multiple versions of a compiler)
installed, you can change the compiler using:

```console
# Run this in your build directory, typically .build
$ CXX=clang++ CC=clang cmake ..

# Then compile and test normally:
$ make -j 4
$ make -j 4 test
```

### Changing the Build Type

By default, the system is compiled with optimizations on; if you want to compile
a debug version, use:

```console
# Run this in your build directory, typically .build
$ CXX=clang++ CC=clang cmake -DCMAKE_BUILD_TYPE=Debug ..

# Then compile and test normally:
$ make -j 4
$ make -j 4 test
```

This project supports `Debug`, `Release` builds

