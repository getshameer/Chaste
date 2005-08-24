# Chaste tests display script.

# This module contains most of the functionality, and is loaded from the
# repository by a wrapper script.

import os, time, pysvn

_standalone = False

#####################################################################
##                          Webpages                               ##
#####################################################################

def index(req):
  """The main test display page.
  
  This displays a summary of the most recent tests.
  """
  page_body = """\
  <h1>Chaste Tests</h1>
  <p>
  This is the funky new interface to Chaste's testing suite.
  </p>
  <ul>
    <li><a href="%s/recent?type=continuous">Recent continuous builds.</a></li>
    <li><a href="%s/recent?type=nightly">Recent nightly builds.</a></li>
  </ul>
  <h2>Latest continuous build</h2>
""" % (_our_url, _our_url) + _summary(req, type='continuous', revision='last')
  
  return _header() + page_body + _footer()


def testsuite(req, type, revision, machine, buildType, testsuite, status, runtime):
  """
  Display the results for the given testsuite, by passing the file back
  to the user.
  """
  req.content_type = 'text/html'
  req.write(_header(), 0)
  test_set_dir = _testResultsDir(type, revision, machine, buildType)
  buildTypesModule = _importBuildTypesModule(revision)
  build = buildTypesModule.GetBuildType(buildType)
  testsuite_file = build.ResultsFileName(test_set_dir, testsuite, status, runtime)
  if os.path.isfile(testsuite_file):
    req.write('\n<pre>\n', 0)
    req.sendfile(testsuite_file)
    req.write('\n</pre>\n', 0)
  else:
    req.write(_error('The requested test suite was not found.'))
  req.write(_footer())


def recent(req, type=''):
  "User-facing page. Content is generated by _recent."
  title = 'Recent '+type+' builds'
  page_body = """\
  <h1>%s</h1>
%s
""" % (title, _recent(req, type))
  return _header(title) + page_body + _footer()

def _recent(req, type=None):
  """Display brief summaries of recent builds of the given type.
  
  Returns a string representing part of a webpage.
  """
  if not type:
    return _error('No type of test to summarise specified.')

  dir = os.path.join(_tests_dir, type)
  if not os.path.isdir(dir):
    return _error(type+' is not a valid type of test.')
  
  # Parse the directory structure within dir into a list of builds
  builds = []
  revisions = os.listdir(dir)
  for revision in revisions:
    machinesAndBuildTypes = os.listdir(os.path.join(dir, revision))
    for machineAndBuildType in machinesAndBuildTypes:
      st = os.stat(os.path.join(dir, revision, machineAndBuildType))
      mod_time = st.st_mtime
      machine, buildType = _extractDotSeparatedPair(machineAndBuildType)
      builds.append([mod_time, int(revision), buildType, machine])
  # Sort the list to be most recent first
  builds.sort()
  builds.reverse()

  output = """\
  <table border="1">
    <tr>
      <th>Date</th>
      <th>Revision</th>
      <th>Build Type</th>
      <th>Machine</th>
      <th>Status</th>
    </tr>
"""
  buildTypesModules = {}
  for build in builds:
    if type == 'nightly':
      date = time.strftime('%d/%m/%Y', time.localtime(build[0]))
    else:
      date = time.strftime('%d/%m/%Y %H:%M:%S', time.localtime(build[0]))
    revision, machine, buildType = build[1], build[3], build[2]
    if not buildTypesModules.has_key(revision):
      buildTypesModules[revision] = _importBuildTypesModule(revision)
    build = buildTypesModules[revision].GetBuildType(buildType)
    test_set_dir = _testResultsDir(type, revision, machine, buildType)
    testsuite_status, overall_status, colour, runtime = _getTestStatus(test_set_dir, build)
    
    output = output + """\
    <tr>
      <td>%s</td>
      <td>%s</td>
      <td>%s</td>
      <td>%s</td>
      <td style="background-color: %s;">%s</td>
    </tr>
""" % (date, _linkRevision(revision), _linkBuildType(buildType, revision),
       machine, colour, _linkSummary(overall_status, type, revision,
                                     machine, buildType))
  output = output + "  </table>\n"
  
  return output



def summary(req, type, revision, machine, buildType):
  "User-facing page. Content is generated by _summary."
  page_body = """\
  <h1>Build Summary</h1>
""" +  _summary(req, type, revision, machine, buildType)
  return _header('Test Summary') + page_body + _footer()
  
def _summary(req, type, revision, machine=None, buildType=None):
  """Display a summary of a build.
  
  Returns a string representing part of a webpage.
  """
  output = ""
  if not (type and revision):
    return _error('No test set to summarise specified.')
  
  # If type=='continuous' and the magic revision 'last' is passed,
  # look for the latest revision present.
  if type == 'continuous' and revision == 'last':
    revisions = os.listdir(os.path.join(_tests_dir, type))
    revision = max(revisions)
    # When getting the latest continuous build, pick the first result
    # set found to determine machine & buildType.
    test_set_dir = os.path.join(_tests_dir, type, revision)
    builds = os.listdir(test_set_dir)
    if len(builds) < 1:
      return _error('No test set found for revision '+revision+
                    '. Probably the build is still in progress.')
    else:
      machine, buildType = _extractDotSeparatedPair(builds[0])
  else:
    if not (machine and buildType):
      return _error('No test set to summarise specified.')
  # Find the directory with appropriate test results
  if type == 'standalone':
    test_set_dir = _dir
  else:
    test_set_dir = _testResultsDir(type, revision, machine, buildType)
  
  # Now test_set_dir should be the directory containing the test results
  # to summarise. Extract summary info from the filenames.
  if type == 'standalone':
    build = _build
  else:
    buildTypesModule = _importBuildTypesModule(revision)
    build = buildTypesModule.GetBuildType(buildType)
  testsuite_status, overall_status, colour, runtime = _getTestStatus(test_set_dir, build)
  
  # Produce output HTML
  output = output + """\
  <p>
  Revision: %s<br />
  Overall status: %s<br />
  Build type: %s<br />
  Machine: %s
  </p>
  <table border="1">
    <tr>
      <th>Test Suite</th>
      <th>Status</th>
      <th>Run Time</th>
    </tr>
""" % (_linkRevision(revision), _colourText(overall_status, colour),
       _linkBuildType(buildType, revision), machine)
  
  # Display the status of each test suite, in alphabetical order
  testsuites = testsuite_status.keys()
  testsuites.sort()
  for testsuite in testsuites:
    output = output + """\
    <tr>
      <td>%s</td>
      <td style="background-color: %s;">%s</td>
      <td>%s</td>
    </tr>
""" % (testsuite, _statusColour(testsuite_status[testsuite], build),
       _linkTestSuite(type, revision, machine, buildType, testsuite,
                      testsuite_status[testsuite], runtime[testsuite], build),
       _formatRunTime(runtime[testsuite]))

  output = output + "  </table>\n"
  
  return output


def buildType(req, buildType, revision=None):
  """
  Display information on the compiler settings, etc. used to build a set
  of tests.
  buildType is the user-friendly name describing these settings, such as
  can be passed to scons build=buildType.
  revision is the code revision of the set of tests, in case the 
  definition of buildType has changed since.
  """
  if revision is None:
    rev = pysvn.Revision(pysvn.opt_revision_kind.head)
    rev_text = ' at the latest revision'
  else:
    rev = pysvn.Revision(pysvn.opt_revision_kind.number, int(revision))
    rev_text = ' at revision %s' % revision
  BuildTypes = _importBuildTypesModule(rev)
  build = BuildTypes.GetBuildType(buildType)
  test_packs = ''
  for test_pack in build.TestPacks():
    test_packs = test_packs + test_pack + ', '
  test_packs = test_packs[:-2]
  # How test suites are run
  testsuite_exe = "testsuite.exe"
  testsuite_cmd = build.GetTestRunnerCommand(testsuite_exe)
  page_body = """\
  <h1>Explanation of build type '%s'%s</h1>
  <p>
  C++ compiler 'brand': %s<br />
  C++ extra compile flags: %s<br />
  Extra linker flags: %s<br />
  Test packs run: %s<br />
  Command to run '%s': %s<br />
  </p>
""" % (buildType, rev_text, build.CompilerType(),
       build.CcFlags(), build.LinkFlags(),
       test_packs, testsuite_exe, testsuite_cmd)
  return _header() + page_body + _footer()


#####################################################################
##                    Helper functions.                            ##
#####################################################################

def _importModuleFromSvn(module_name, module_filepath,
                         revision=pysvn.Revision(pysvn.opt_revision_kind.head)):
  """
  Use pysvn and imp to import the requested revision of the given
    module from the repository.
  module_name is the name to give the module.
  module_filepath is the path to the module file within the trunk
    directory of the repository.
  By default import the latest version.
  Return the module object.
  """
  filepath = _svn_repos + module_filepath
  client = _svnClient()
  module_text = client.cat(filepath, revision)
  return _importCode(module_text, module_name)

def _importBuildTypesModule(revision=pysvn.Revision(pysvn.opt_revision_kind.head)):
  """
  Use pysvn and imp to import the requested revision of the BuildTypes.py
  module from the repository.
  By default import the latest version.
  """
  if type(revision) in [type(""), type(1)]:
    revision = pysvn.Revision(pysvn.opt_revision_kind.number, int(revision))
  return _importModuleFromSvn('BuildTypes', '/python/BuildTypes.py', revision)

def _svnClient():
  "Return a pysvn.Client object for communicating with the svn repository."
  client = pysvn.Client()
  def get_login(realm, username, may_save):
    return True, _svn_user, _svn_pass, True
  client.callback_get_login = get_login
  # We trust our server even though it has a dodgy certificate
  # Might want to make this a bit more secure?
  def ssl_server_trust_prompt(trust_dict):
    return True, trust_dict['failures'], True
  client.callback_ssl_server_trust_prompt = ssl_server_trust_prompt
  return client

def _importCode(code, name, add_to_sys_modules=0):
  """
  Import dynamically generated code as a module. code is the
  object containing the code (a string, a file handle or an
  actual compiled code object, same types as accepted by an
  exec statement). The name is the name to give to the module,
  and the final argument says wheter to add it to sys.modules
  or not. If it is added, a subsequent import statement using
  name will return this module. If it is not added to sys.modules
  import will try to load it in the normal fashion.
  Code from the Python Cookbook.
  
  import foo
  
  is equivalent to
  
  foofile = open("/path/to/foo.py")
  foo = importCode(foofile,"foo",1)
  
  Returns a newly generated module.
  """
  import sys, imp
  
  module = imp.new_module(name)
  
  exec code in module.__dict__
  if add_to_sys_modules:
    sys.modules[name] = module
    
  return module

def _extractDotSeparatedPair(string):
  """
  Extract both parts from a string of the form part1.part2.
  The '.' used is the last in the string.
  Returns a pair (part1, part2).
  Useful for parsing machine.buildType filenames.
  """
  i = string.rfind('.')
  return string[:i], string[i+1:]

def _testResultsDir(type, revision, machine, buildType):
  """
  Return the directory in which test results are stored for this
  test type, code revision, build machine and build type.
  """
  return os.path.join(_tests_dir, type, str(revision), machine+'.'+buildType)

def _getTestStatus(test_set_dir, build):
  """
  Return the status for all tests in the given directory, and compute
  a summary status given the build type.
  Return a triple (dict, string, string) where the first entry maps
  test suite names to a string describing their status, the second is
  the overall status, and the third is the colour in which to display
  the overall status.
  """
  ignores = ['index.html', '.sconsign']
  result_files = os.listdir(test_set_dir)
  testsuite_status, runtime = {}, {}
  for filename in result_files:
    if not filename in ignores:
      d = build.GetInfoFromResultsFileName(filename)
      testsuite = d['testsuite']
      testsuite_status[testsuite] = d['status']
      runtime[testsuite] = d['runtime']
  overall_status, colour = _overallStatus(testsuite_status.values(),
                                          build)
  return testsuite_status, overall_status, colour, runtime

def _overallStatus(statuses, build):
  """
  Given a list of the status of each test suite, and the type of build
  performed, return the overall status.
  Return value is a pair, the first item of which is a string given
  the number of failing test suites, and the second a colour name.
  """
  total = len(statuses)
  failed = 0
  for status in statuses:
    if not build.IsGoodStatus(status):
      failed = failed + 1
  if failed > 0:
    result = "Failed %d out of %d test suites" % (failed, total)
    colour = "red"
  else:
    result = "All tests passed"
    colour = "green"
  return result, colour

def _statusColour(status, build):
  """
  Return the name of the colour in which this status string should be
  displayed, given that the build type was build.
  """
  if build.IsGoodStatus(status):
    colour = "green"
  else:
    colour = "red"
  return colour

#####################################################################
##                   HTML helper functions.                        ##
#####################################################################

def _linkRevision(revision):
  "Return a link tag to the source browser for this revision."
  if revision == 'working copy':
    return revision
  return '<a href="%s?rev=%s">%s</a>' % (_source_browser_url,
                                         revision, revision)

def _linkBuildType(buildType, revision):
  "Return a link tag to the detailed info page for this build type."
  if revision == 'working copy':
    return buildType
  else:
    query = 'buildType?buildType=%s&revision=%s' % (buildType, revision)
    return '<a href="%s/%s">%s</a>' % (_our_url, query, buildType)

def _linkSummary(text, type, revision, machine, buildType):
  """
  Return a link tag to the summary page for this set of tests.
  text is the text of the link.
  """
  query = 'type=%s&revision=%s&machine=%s&buildType=%s' % (type, revision,
                                                           machine, buildType)
  return '<a href="%s/summary?%s">%s</a>' % (_our_url, query, text)

def _linkTestSuite(type, revision, machine, buildType, testsuite,
                   status, runtime, build):
  """
  Return a link tag to a page displaying the output from a single
  test suite.
  """
  if type == 'standalone':
    filename = build.ResultsFileName(os.curdir, testsuite, status, runtime)
    link = '<a href="%s">%s</a>' % (filename, build.DisplayStatus(status))
  else:
    query = 'type=%s&revision=%s&machine=%s&buildType=%s' % (type, revision,
                                                             machine, buildType)
    query = query + '&testsuite=%s&status=%s&runtime=%d' % (testsuite, status, runtime)
    link = '<a href="%s/testsuite?%s">%s</a>' % (_our_url, query, 
                                                 build.DisplayStatus(status))
  return link

def _formatRunTime(runtime):
  "Return a human-readable version of the given runtime (which is in s)."
  if runtime < 0:
    s = 'Unknown'
  elif runtime < 60:
    s = str(runtime) + 's'
  else:
    # Minutes & seconds
    s = "%d:%02d" % (runtime // 60, runtime % 60)
  return s

def _colourText(text, colour):
  "Return text in the given colour."
  return '<span style="color: %s;">%s</span>' % (colour, text)

def _error(msg):
  "Encapsulate an error message."
  return '<p class="error">%s</p>' % msg

def _header(title=""):
  """HTML page header."""
  if title:
    title = " - " + title
  header = """\
<html>
  <head>
    <title>Chaste Tests%s</title>
  </head>
  <body>""" % title
  return header

def _footer():
  """HTML page footer."""
  footer = """\
  <hr />
  <a href="%s">Tests index page</a><br />
  <a href="https://chaste.ediamond.ox.ac.uk/cgi-bin/trac.cgi">Chaste project website</a>
  </body>
</html>""" % _our_url
  return footer


#####################################################################
##                     Standalone version                          ##
#####################################################################

if __name__ == '__main__':
  # We're being run from the command line rather than from within mod_python
  # Arguments should be:
  #  1: the directory containing test output files
  #  2: the type of build (string, defaults to 'default')
  # We write an index.html file for the local run of tests in the given directory.
  _standalone = True
  import sys, BuildTypes, socket
  if len(sys.argv) < 2:
    print "Syntax error."
    print "Usage:",sys.argv[0],"<test output dir> [<build type>]"
    sys.exit(1)
  _dir = sys.argv[1]
  if len(sys.argv) > 2:
    _build_type = sys.argv[2]
  else:
    _build_type = 'default'
  _build = BuildTypes.GetBuildType(_build_type)
  _machine = socket.getfqdn()

  # Alter the configuration slightly
  _tests_dir = '.'
  _source_browser_url = 'https://comlab2.lsi.ox.ac.uk/cgi-bin/trac.cgi/browser/'
  _our_url = 'https://comlab2.lsi.ox.ac.uk/tests.py'

  _fp = file(os.path.join(_dir, 'index.html'), 'w')

  print >>_fp,_header('Test Summary For Local Build')
  print >>_fp,'<h1>Test Summary For Local Build</h1>'
  print >>_fp,'<p>Displaying info for tests with output stored in',_dir,'</p>'
  print >>_fp,_summary(None, 'standalone', 'working copy', _machine, _build_type)
  print >>_fp,_footer()
  _fp.close()

  print "Test summary generated in", os.path.join(_dir, 'index.html')
