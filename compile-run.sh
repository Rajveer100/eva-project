# Compile file main:
clang++ -o eva $(/opt/homebrew/Cellar/llvm/17.0.6_1/bin/llvm-config --cxxflags --ldflags --system-libs --libs core) -std=c++2b Eva.cpp

# Run main:
./eva

# Execute generated IR:
/opt/homebrew/Cellar/llvm/17.0.6_1/bin/lli out.ll

# Print result:
echo $?

printf "\n"