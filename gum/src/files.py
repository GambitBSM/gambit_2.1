"""
Master module containing all routines for finding, creating,
amending, and reading files.
"""

import os
import re
import numpy as np
import yaml
from distutils.dir_util import remove_tree
from collections import defaultdict

from setup import *

def remove_tree_quietly(path):
    """
    Deletes a directory if it exists
    """
    if os.path.exists(path):
        remove_tree(path)

def mkdir_if_absent(path):
    """
    Makes a new directory if the proposed path doesn't yet exist
    """
    try:
        os.makedirs(path)
    except OSError:
        if not os.path.isdir(path):
            raise

def full_filename(filename, module):
    """
    Formats a gambit file correctly based on the filename, the
    module, and whether it is a header file.
    """

    # strip leading & trailing slashes
    filename.strip('/')
    module.strip('/')

    path = ""
    if filename.endswith(".hpp"):
        path = "include/gambit/{0}/".format(module)
    elif filename.endswith(".cpp"):
        path = "src/"
    elif filename.endswith(".py"):
        path = "scripts/"
    elif filename.endswith(".dif"):
        path = "patches/"
    else:
        path = ""

    location = "../{0}/{1}{2}".format(module, path, filename)
    return location

def find_file(filename, module):
    """
    Tries to find a file in a specified module.
    """

    location = full_filename(filename, module)

    if os.path.exists(location):
        return True
    else:
        return False

def find_capability(capability, module, filename=None):
    """
    Tries to find a CAPABILITY in a specified rollcall file.
    """

    module.strip('/')

    if not filename:
        filename = "{0}_rollcall.hpp".format(module)
    else:
        filename.strip('/')

    location = full_filename(filename, module)
    if find_file(filename, module):
        pass
    else:
        raise GumError(("\n\nCannot find capability {0} in rollcall header file"
                        " {1} as the file does not exist!!").format(capability,
                                                                    location))

    lookup = "#define CAPABILITY " + capability

    with open(location) as f:
        for num, line in enumerate(f, 1):
            if lookup in line:
                return True, num

    return False, 0

def amend_rollcall(capability, module, contents, reset_dict, filename=None):
    """
    Adds a new FUNCTION to an existing CAPABILITY in a rollcall header.
    """

    module.strip('/')

    if not filename:
        filename = "{0}_rollcall.hpp".format(module)
    else:
        filename.strip('/')

    location = full_filename(filename, module)
    if find_file(filename, module):
        pass
    else:
        raise GumError(("\n\nCannot find capability {0} in rollcall header file"
                        " {1} as the file does not exist!!").format(capability,
                                                                    location))

    lookup = "#define CAPABILITY " + capability

    found = False

    with open(location) as f:
        for num, line in enumerate(f, 1):
            if lookup in line:
                found = True
                break

    lookup = "#undef CAPABILITY"

    # Found the capability -> find the #undef
    if found == True:
        with open(location) as f:
            for i in xrange(num):
                f.next()
            for no, line in enumerate(f, 1+num):
                if lookup in line:
                    break
            amend_file(filename, module, contents, no-1, reset_dict)
    else:
        raise GumError(("\n\nCapability {0} not found in "
                        "{1}!").format(capability, filename))

def find_function(function, capability, module, filename=None):
    """
    Tries to find a FUNCTION in a specified rollcall header file.
    """

    module.strip('/')

    if not filename:
        filename = "{0}_rollcall.hpp".format(module)
    else:
        filename.strip('/')

    location = full_filename(filename, module)

    # First check the capability exists...
    exists, num = find_capability(capability, module, filename)

    lookup = "#define FUNCTION " + function
    terminate = "#undef CAPABILITY"

    with open(location) as f:
        for i in xrange(num):
            f.next()
        for no, line in enumerate(f, 1+num):
            if lookup in line:
                return True, no
            if terminate in line:
                return False, 0

    return False, 0

def find_string(filename, module, string):
    """
    Tries to find a generic string in a given file.
    """

    location = full_filename(filename, module)

    if find_file(filename, module):
        pass
    else:
        raise GumError(("\n\nCannot find string {0} in the file"
                        " {1} as the file does not exist!!").format(string,
                                                                    location))

    with open(location) as f:
        for num, line in enumerate(f, 1):
            if string in line:
                return True, num

    return False, 0

def write_file(filename, module, contents, reset_dict):
    """
    Writes a file in a specified location.
    """

    location = full_filename(filename, module)
    location_parts = os.path.split(location)

    if find_file(filename, module):
        raise GumError(("\n\nTried to write file " + location +
                        ", but it already exists."))

    reset_dict['new_files']['files'].append(location)

    # Make the directory if it doesn't exist
    mkdir_if_absent(location_parts[0])
    # Create new file
    open(location, 'w').write(contents)

    print("File {} successfully created.".format(location))

def copy_file(filename, module, output_dir, reset_dict, existing=True):
    """
    Copies an output file in a specified location.
    """
    import shutil

    location = full_filename(filename, module)
    location_parts = os.path.split(location)
    GUM_version = output_dir + '/' + location[3:]

    # Todo - do something with this. It's not doing anything right now.
    if existing:
        reset_dict['copied_amended_files']['files'].append(location)
    else:
        reset_dict['new_files']['files'].append(location)

    mkdir_if_absent(location_parts[0])
    # Copy the file
    shutil.copy(GUM_version, location)

    print("Copied "+GUM_version+" to "+location+".")

def delete_file(filename, module):
    """
    Deletes a file in a specified location.
    """

    location = full_filename(filename, module)

    if find_file(filename, module):
        os.remove(location)
        print("File {} successfully removed.".format(location))

def amend_file(filename, module, contents, line_number, reset_dict):
    """
    Amends a file in a specified location with 'contents', starting
    from a given line number.
    """

    location = full_filename(filename, module)

    if not find_file(filename, module):
        raise GumError(("\n\nERROR: Tried to amend file " + location +
                        ", but it does not exist."))

    # Get the indentation out the front of the contents

    reset_dict['amended_files'][location].append(contents)

    temp_location = location + "_temp"
    lines = open(location, 'r').readlines()

    with open(temp_location, 'w') as f:
        for i in xrange(line_number):
            f.write(lines[i])
        f.write(contents)
        for i in xrange(len(lines)-line_number):
            f.write(lines[i+line_number])

    os.remove(location)
    os.rename(temp_location, location)

    print("File {} successfully amended.".format(location))

def write_capability(capability, functions):
    """
    Writes a new capability, wrapped around (a) function(s),
    for a rollcall header file.
    """

    towrite = (
            "  #define CAPBILITY {0}\n"
            "  START_CAPABILITY\n"
            "  \n"
            "{1}"
            "  \n"
            "  #undef CAPABILITY\n"
    ).format(capability, '\n'.join(functions))

    print("Capability {} added.".format(capability))

    return towrite

def write_function(function, returntype, dependencies=None,
                   allowed_models=None, backend_reqs=None):
    """
    Writes a function for a rollcall header file.
    """

    extras = ""

    if dependencies:
        for i in np.arange(len(dependencies)):
            extras += "DEPENDENCY({0}, {1})\n".format(dependencies[i][0],
                                                      dependencies[i][1])
    if allowed_models:
        extras += "ALLOW_MODELS({0})\n".format(', '.join(allowed_models))

    if backend_reqs:
        for i in np.arange(len(backend_reqs)):
            bereq = backend_reqs[i]
            if not isinstance(bereq, BackendReq):
                raise GumError(("\n\nBackend requirement at position " + i +
                                "not passed as instance of class BackendReq."))

            add = ", ".join([bereq.capability,
                             "({0})".format(', '.join(bereq.tags)),
                             bereq.cpptype])

            if not bereq.var:
                add += ", ({0})".format(', '.join(bereq.args))

            extras += "BACKEND_REQ({0})\n".format(add)

    towrite = (
            "\n"
            "#define FUNCTION {0}\n"
            "START_FUNCTION({1})\n"
            "{2}"
            "#undef FUNCTION\n"
    ).format(function, returntype, extras)

    return dumb_indent(4, towrite)

def blame_gum(message):
    """
    Writes function to dump at the beginning of a new GAMBIT file.
    Blames GUM. Takes a message to describe the new file.
    """

    towrite = (
            "//   GAMBIT: Global and Modular BSM Inference Tool\n"
            "//   *********************************************\n"
            "///  \\file\n"
            "///\n"
            + message +
            "\n"
            "///\n"
            "///  Authors (add name and date if you modify):    \n"
            "///       *** Automatically created by GUM ***     \n"
            "///                                                \n"
            "///  \\author The GAMBIT Collaboration             \n"
            "///  \date " +
            datetime.datetime.now().strftime("%I:%M%p on %B %d, %Y") +
            "\n"
            "///                                                \n"
            "///  ********************************************* \n"
            "\n"
    )

    return towrite

def dumb_indent(numspaces, text):
    """
    Indents input text by specified number of spaces.
    """

    lines = text.splitlines(True)
    out = ""
    for line in lines:
       for i in range(0, numspaces):
           out += " "
       out += line

    return out

def indent(text, numspaces=0):
    """
    Increases indents of text every time it detects an open
    curly brace {, and decreases every time it detects a closed
    curly brace }. Does nothing if there is a {}.
    """

    lines = text.splitlines(True)
    out = ""

    for line in lines:


        if numspaces < 0:
            raise GumError(("\n\nTried to indent a negative number of spaces."
                            "Please check for rogue braces."))

        num_open = line.count("{")
        num_closed = line.count("}")
        if num_open != num_closed:
            numspaces -= (num_closed*2)
        for i in range(0, numspaces):
            out += " "
        if num_open != num_closed:
            numspaces += (num_open*2)
        out += line

    return out

def reformat(location):
    """
    Reformat all text in a file (indents, etc.)
    """

def revert(reset_file):
    """
    Go back to the previous save point.
    """

    print("GUM called in reset mode.")

    if not reset_file.endswith('.mug'):
        raise GumError(("\n\n\tPlease use a generated .mug "
                       "file if using the reset function."))

    with open(reset_file, "r") as f:

        try:
            data = yaml.safe_load(f)
        except yaml.YAMLError as exc:
            print(exc)
            return

        # The files GUM wrote as new.
        # GUM can just simply delete these.
        new_files = data['new_files']['files']


        for i in new_files:
            if os.path.isfile(i):
                print("Deleting {0}...".format(i))
                os.remove(i)
            else:
                print("Tried deleting {0}, but it seems to have already been removed.".format(i))

        # The files that existed previously, that GUM added stuff to.
        # These are a little more annoying to deal with.
        amended_files = data['amended_files']

        # We want to match *strings* and not line numbers or anything like that.
        # This way, there is no order needed to perform resets in.

        for filename, v in amended_files.iteritems():

            temp_file = filename + "_temp"
            with open(filename, 'r') as original_file:
                text = original_file.read()
                lines = text.splitlines()

            # This is the YAML node for the strings needing to be deleted
            for string in v:

                # If we find it, kill it
                if string in text:
                    text = text.replace(string, '')
                # Otherwise assume someone has messed with it
                else:
                    print(("Tried deleting the following entry from "
                           "{0}, but I could not find it:\n\n{1}\n\n"
                           "Perhaps it has already been removed."
                           ).format(filename, string))

            # Write the new amended file
            new_file = open(temp_file, 'w')
            new_file.write(text)

            os.remove(filename)
            os.rename(temp_file, filename)

    return

def check_for_existing_entries(model_name, darkbit, colliderbit, output_opts):
    """
    Checks for existing entries within GAMBIT, for a new model.
    """

    # Models entries
    if ( find_file("models/" + model_name + ".hpp", "Models") or
         find_file("SpectrumContents/" + model_name + ".cpp", "Models") or 
         find_file("SimpleSpectra/" + model_name + "SimpleSpec" + ".hpp", "Models") or 
         find_file("SpecBit_" + model_name + ".cpp", "SpecBit") or 
         find_file("SpecBit_" + model_name + "_rollcall.hpp", "SpecBit")
       ):
        raise GumError(("Model {0} already exists in the Model Hierarchy.").format(model_name))

    if darkbit:
        if find_file(model_name + ".cpp", "DarkBit"):
            raise GumError(("Model {0} already exists in DarkBit.").format(model_name))
        if output_opts.mo:
            ver = "3.6.9.2"
            f = "frontends/MicrOmegas_{0}_{1}".format(model_name, ver.replace('.','_'))
            if ( find_file(f+".cpp", "Backends") or 
                 find_file(f+".hpp", "Backends")
               ):
                raise GumError(("MicrOmegas entry already exists for model {0}").format(model_name))

def drop_mug_file(mug_file, contents):
    """
    Drops a .mug (reset) file from the reset contents saved by GUM.
    """

    d = dict(contents)

    new_files = dict(d['new_files'])
    amended_files = dict(d['amended_files'])

    new_contents = {'new_files': new_files, 'amended_files': amended_files}

    with open(mug_file, 'w') as f:
        yaml.dump(new_contents, f, default_flow_style=False)


def drop_yaml_file(model_name, model_parameters, add_higgs, reset_contents):
    """
    Drops an example YAML file with all decays of a new model
    added.
    """

    towrite = (
        "##########################################################################\n"
        "## GAMBIT configuration for a random scan of the {0} model\n"
        "##\n"
        "## Includes simply the decays of the new model.\n"
        "##########################################################################\n"
        "\n"
        "\n"
        "Parameters:\n"
        "\n"
        "  # SM parameters.\n"
        "  StandardModel_SLHA2: !import include/StandardModel_SLHA2_defaults.yaml\n"
        "\n"
    ).format(model_name)

    if add_higgs:
        towrite+= (
            "  StandardModel_Higgs:\n"
            "    mH: 125.09\n"
            "\n"
        )

    towrite += ("  {0}:\n").format(model_name)

    # Don't want the SM-like Higgs mass a fundamental parameter
    bsm_params = [x for x in model_parameters if x.name != 'h0_1' and x.sm == False]
    params = []

    for i in bsm_params:
        if i.shape == 'scalar' or i.shape == None: params.append(i.gb_in)
        elif re.match("m[2-9]x[2-9]", i.shape): 
            size = int(i.shape[-1])
            for j in xrange(size):
                for k in xrange(size):
                    params.append(i.gb_in + str(j+1) + 'x' + str(k+1))
        elif re.match("v[2-9]", i.shape): 
            size = int(i.shape[-1])
            for j in xrange(size):
                params.append(i.gb_in + str(j+1))

    # No double counting (also want to preserve the order)
    norepeats = []
    [norepeats.append(i) for i in params if not i in norepeats]

    for i in norepeats:
        towrite += ("    {0}: 0.1\n").format(i)

    towrite += (
        "Priors:\n"
        "\n"
        "  # All the priors are simple for this scan, so they "
        "are specified directly in the Parameters section.\n"
        "\n"
        "\n"
        "Printer:\n"
        "\n"
        "  printer: hdf5\n"
        "\n"
        "  options:\n"
        "    output_file: \"{0}.hdf5\"\n"
        "    group: \"/{0}\"\n"
        "\n"
        "\n"
        "Scanner:\n"
        "\n"
        "  use_scanner: random\n"
        "\n"
        "  scanners:\n"
        "\n"
        "    random:\n"
        "      plugin: random\n"
        "      point_number: 10\n"
        "      like:  LogLike\n"
        "\n"
        "ObsLikes:\n"
        "\n"
        "  - purpose:      Observable\n"
        "    capability:   decay_rates\n"
        "    type:         DecayTable\n"
        "    printme:      false\n"
        "\n"
        "Rules:\n"
        "\n"
        "  # Choose to get decays from DecayBit proper, not from an SLHA file.\n"
        "  - capability: decay_rates\n"
        "    function: all_decays\n"
        "Logger:\n"
        "\n"
        "  redirection:\n"
        "    [Debug] : \"debug.log\"\n"
        "    [Default] : \"default.log\"\n"
        "    [DecayBit] : \"DecayBit.log\"\n"
        "    [PrecisionBit] : \"PrecisionBit.log\"\n"
        "    [ColliderBit] : \"ColliderBit.log\"\n"
        "    [SpecBit] : \"SpecBit.log\"\n"
        "    [Dependency Resolver] : \"dep_resolver.log\"\n"
        "\n"
        "KeyValues:\n"
        "\n"
        "  dependency_resolution:\n"
        "    prefer_model_specific_functions: true\n"
        "\n"
        "  likelihood:\n"
        "    model_invalid_for_lnlike_below: -5e5\n"
        "    model_invalid_for_lnlike_below_alt: -1e5\n"
        "\n"
        "  default_output_path: \"runs/{0}/\"\n"
        "\n"
        "  debug: false\n"
        "\n"
    ).format(model_name)

    write_file(model_name + '.yaml', 'yaml_files', towrite, reset_contents)

def write_config_file(outputs, model_name, reset_contents):
    """
    Drops a configuration file, which will build the correct backends, 
    and then GAMBIT, in the correct order.
    """

    towrite = (
        "cd ../build\n"
    )

    if outputs.pythia:
        towrite += (
            "cmake ..\n"
            "make -j4 pythia_{0}\n"
        ).format(model_name.lower())

    if outputs.mo:
        towrite += (
            "cmake ..\n"
            "make -j4 micromegas\n"
        )

    # spheno, flexiblesusy, etc.

    towrite += (
        "cmake ..\n"
        "make -j4 gambit\n"
    )

    write_file(model_name + '_config.sh', 'gum', towrite, reset_contents)