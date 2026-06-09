file(REMOVE_RECURSE
  "libprotocols.a"
  "libprotocols.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/protocols.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
