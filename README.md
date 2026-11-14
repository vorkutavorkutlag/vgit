# VGIT

The bare-bones local version control tool.
Doesn't have the rich features of git, much more lightweight and honestly for personal use.

What better way to learn a system than to try building it yourself?


Example usage:
```
Quick guide:
vgit init
vgit branch main
vgit add *
vgit commit -m 'first commit'


Usage: [OPTIONS] [SUBCOMMANDS]


OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  init                        Initialize repository
  branch                      Manage branches
  switch                      Switch branch
  add                         Add files to stage
  reset                       Reset files from stage
  diff                        Show files on stage
  commit                      Commit current changes in stage
  history                     View commit history in branch
  rollback                    Roll back to previous commit
  nuke                        Delete repository in working directory
```
