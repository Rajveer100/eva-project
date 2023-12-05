# Compile file main:
clang++ -o eva $(/opt/homebrew/Cellar/llvm/17.0.6/bin/llvm-config --cxxflags --ldflags --system-libs --libs core) Eva.cpp

# Run main:
./eva

# Execute generated IR:
/opt/homebrew/Cellar/llvm/17.0.6/bin/lli out.ll

# Print result:
echo $?

printf "\n"