# Define options for the different optimizations
option(OPTIMIZATION1 "Enable optimization 1 (UCP Memory Allocation)" OFF)
option(OPTIMIZATION2 "Enable optimization 1 (UCP Memory Allocation) and 2 (warmup iteration)" OFF)

# Add the necessary defines based on the values of the options
if (OPTIMIZATION1)
    add_definitions("-DOPTIMIZATION_1=ON")
endif (OPTIMIZATION1)
if (OPTIMIZATION2)
    add_definitions("-DOPTIMIZATION_2=ON")
endif (OPTIMIZATION2)