Contributing To Glaxnimate
=======================================


Donating
---------------------------------------

If you don't have any technical skill you can still support Glaxnimate by
[donating](https://glaxnimate.mattbas.org/donate).


Reporting Issues
---------------------------------------

You can report issues on [GitLab](https://gitlab.com/mattbas/glaxnimate/-/issues).

Bug reports and feature requests are welcome.

### Bugs

When reporting bugs please be as detailed as possible, include steps to reproduce
the issue and describe what happens.

If relevant, attach the file you were editing on the issue.

You can also add the system information gathered by Glaxnimate itself:
Go to *Help > About... > System* and click *Copy*,
then you can paste on the issue the system information.


Packaging
---------------------------------------

You can create packages to port Glaxnimate to a specific system,
setting up an automatable process to create said package so it can be
integrated with continuous integration.


### Packages that need help

* Android: Needs setting up a build pipeline and be ported to a modern version of Qt
* Mac DMG: Needs to include dependencies as framework
* Windows: Needs reducing the shipped zip size and fixing platform-specific issues

### Existing Packages

* AppImage
* Snap
* Deb
* AUR
* PyPI
* Itch.io

### More packages

If you want to port Glaxnimate to a different system of package manager,
feel free to do so!

If the process can be automated the script can be added to continuous integration
so it gets built automatically.


Documentation
---------------------------------------

You can add to the documentation by adding tutorials, missing information,
correcting typos, etc...

On the [Documentation Website](https://glaxnimate.mattbas.org/) each page
has a link to its source file on GitLab, you can use that page to edit it and
create a pull request.

Details on how to work with the documentation are at [Documentation](https://glaxnimate.mattbas.org/contributing/documentation/).


Translations
---------------------------------------

You can add or improve translations using Qt linguist.

See [Translations](https://glaxnimate.mattbas.org/contributing/translations/) for more details.


Code
---------------------------------------

See the [README](https://glaxnimate.mattbas.org/contributing/read_me/) for build instructions.

You can open [merge requests on GitLab](https://gitlab.com/mattbas/glaxnimate/-/merge_requests)
to get your changes merged into Glaxnimate.

### License

Glaxnimate is licensed under the [GNU GPLv3+](http://www.gnu.org/licenses/gpl-3.0.html),
so your contributions must be under the same license.


Credits and Licensing
---------------------------------------

If you make significant contributions add your name or nickname in the appropriate section of AUTHORS.md,
you can also include your email but you don't have to.

If you contribute to the Glaxnimate, you agree that your contributions are
compatible with the licensing terms.

For documentation contributions the license is dual GPLv3+ and CC BY-SA 4.0.

For everything else in the Glaxnimate repository, the license is GPLv3+.

Some submodules have their own licensing terms.
