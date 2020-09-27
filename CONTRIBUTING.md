Contributing To Glaxnimate
=======================================


Donating
---------------------------------------

If you don't have any technical skill you can still support Glaxnimate by
[donating](https://glaxnimate.mattbas.org/donate).


Reporting Issues
---------------------------------------

You can report issues on [GitLab](https://gitlab.com/mattia.basaglia/glaxnimate/-/issues).

Bug reports and feature requests are welcome.

### Bugs

When reporting bugs please be as detailed as possible, include steps to reproduce
the issue and describe what happens.

If relevant, attach the file you were editing on the issue.

You can also add the system information gathered by Glaxnimate itself:
Go to *Help > About... > System* and click *Copy*,
then you can paste on the issue the system information.


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

### Adding Languages

To add a new translation, edit the file `data/CMakeLists.txt` to add
the translation file in the form `translations/glaxnimate_(code).ts`,
where *(code)* is a locale code (eg: **en**, **en_US**, ...).

Then run `make translations` for the file to be generated.

### Editing Translations

To edit a translation file, you can open it with Qt Linguist, which should
show context information and the string in the GUI when needed.

The translation files are under `data/translations`.

After a file has been edited, `make translations` will build the output file
and the translation should be available in Glaxnimate settings.


Code
---------------------------------------

See the [README](https://glaxnimate.mattbas.org/contributing/read_me/) for build instructions.

You can open [merge requests on GitLab](https://gitlab.com/mattia.basaglia/glaxnimate/-/merge_requests)
to get your changes merged into Glaxnimate.

### License

Glaxnimate is licensed under the [GNU GPLv3+](http://www.gnu.org/licenses/gpl-3.0.html),
so your contributions must be under the same license.

### Cross Compiling

See [Cross Compiling](https://glaxnimate.mattbas.org/contributing/cross_compiling/)
for intructions of how to build Glaxnimate using MXE.
