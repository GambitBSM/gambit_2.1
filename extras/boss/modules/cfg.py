##########################################
#                                        #
#  Global configuration module for BOSS  #
#                                        #
##########################################

#
# Variables in this module can be accessed and altered by 
# all other modules. The values listet here are only default
# values.
#

from collections import OrderedDict


xml_file_name    = ''
id_dict          = OrderedDict() 
file_dict        = OrderedDict()
std_types_dict   = OrderedDict()
typedef_dict     = OrderedDict()
class_dict       = OrderedDict()
func_dict        = OrderedDict()
new_header_files = OrderedDict()
accepted_types   = [] 
std_headers_used = []

std_types_in_class = {}
all_types_in_class = {}

# accepted_paths     = ['pythia8186_original', 'pythia8186']
accepted_paths     = ['minimal_original', 'minimal']

std_include_paths  = ['/usr/include/']

# loaded_classes       = ['Pythia8::Pythia', 'Pythia8::Hist', 'Pythia8::Event', 'Pythia8::Particle', 'Pythia8::Info', 'Pythia8::Vec4']
# loaded_functions     = ['Pythia8::m']
loaded_classes       = ['X', 'DummyNamespace::Y']
loaded_functions     = []

wrapper_class_tree     = True
load_parent_classes    = False
wrap_inherited_members = False

extra_output_dir        = 'output'
code_suffix             = '_GAMBIT'
abstr_header_prefix     = 'Abstract_'
factory_file_prefix     = 'factory_'
abstr_class_prefix      = 'Abstract_'
wrapper_header_prefix   = 'GAMBIT_wrapper_'

all_wrapper_fname       = 'boss_loaded_classes'
wrapper_deleter_fname   = 'wrapper_deleter'
all_typedefs_fname      = 'all_typedefs'
frwd_decls_abs_fname    = 'forward_decls_abstract_classes'
wrapper_typedefs_fname  = 'GAMBIT_wrapper_typedefs'

header_extension        = '.hpp'
source_extension        = '.cpp'

add_path_to_includes    = '' #'Pythia8'


indent = 4


# Dictionary of what header to include for various standard types
known_class_headers = {
    "std::array"             : "<array>", 
    "std::vector"            : "<vector>", 
    "std::deque"             : "<deque>", 
    "std::list"              : "<list>", 
    "std::forward_list"      : "<forward_list>", 
    "std::set"               : "<set>",  
    "std::multiset"          : "<set>", 
    "std::map"               : "<map>", 
    "std::multimap"          : "<map>", 
    "std::unordered_set"     : "<unordered_set>", 
    "std::unordered_multiset": "<unordered_set>", 
    "std::unordered_map"     : "<unordered_map>", 
    "std::unordered_multimap": "<unordered_map>", 
    "std::stack"             : "<stack>", 
    "std::queue"             : "<queue>",
    "std::priority_queue"    : "<queue>",
    "std::string"            : "<string>",
    "std::istream"           : "<istream>",
    "std::ostream"           : "<ostream>",
    "std::iostream"          : "<iostream>"
}


operator_names = {
    "="   : "assignment",
    "+"   : "addition",
    "-"   : "subtraction",
    "*"   : "multiplication",
    "/"   : "division",
    "%"   : "modulo",
    # "++"  : "increment",  # Need to differentiate between ++i and i++. Commented out for now.
    # "--"  : "decrement",
    "+="  : "addition_assignment",
    "-="  : "subtraction_assignment",
    "*="  : "multiplication_assignment",
    "/="  : "division_assignment",
    "%="  : "modulo_assignment",
    "[]"  : "array_subscript",
    "()"  : "function_call",
}
