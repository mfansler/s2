## Tweaks to the C++ source code are run in this R script.
## Thus running this (inside the current tools dir) after copying the
## geometry dir from s2 sources should yield a working C++ lib with R.
## IT IS IMPORTANT TO EXECUTE COMMANDS IN ORDER SO DON'T SKIP ANY!!

# Copy geometry dir from upstream to src and move headers to inst/include.
# Also delete obsolote python and tests dirs.
# Here are some potentially useful commands for moving headers
# > find . -name '*.h' -print | cpio -pvdumB ../../inst/include/
# > find . -name '*.h' -exec rm -f {} +

## Move to geometry dir:
owd <- setwd(file.path("..", "src", "geometry"))

###### GIT: Use Rcpp to handle cout, cerr and abort. #####
## Some simple tweaks using git to find matching files and sed to substitute code
## in geometry dir:
system("git grep -l 'using std::cout' | xargs sed -i '/using std::cout/d'")
system("git grep -l 'std::cerr' | xargs sed -i 's/std::cerr/Rcpp::Rcerr/g'")
system("git grep -l 'cout' | xargs sed -i 's/cout/Rcpp::Rcout/g'")
## Repeat for headers in inst/include dir:
system("git grep -l 'using std::cout' ../../inst | xargs sed -i '/using std::cout/d'")
system("git grep -l 'std::cerr' ../../inst | xargs sed -i 's/std::cerr/Rcpp::Rcerr/g'")
system("git grep -l 'cout' ../../inst | xargs sed -i 's/cout/Rcpp::Rcout/g'")

## Fix abort() call and include Rcpp.h in base/logging.h
file <- file.path("..", "..", "inst", "include", "base", "logging.h")
content <- readLines(file)
content <- gsub('abort()', 'Rcpp::stop("An error has occured in the C++ library!")', content, fixed = TRUE)
ii <- grep('define BASE_LOGGING_H', content)
# Add lines
content <- append(content, c('', '#include <Rcpp.h>'), ii)
writeLines(content, file)
############################################################

###### GIT: Tweak for Windows compiling. ########
## Fix Windows byte swap in base/port.h
file <- file.path("..", "..", "inst", "include", "base", "port.h")
content <- readLines(file)
ii <- grep('define __BYTE_ORDER for MSVC', content)
# Overwrite next line
content[ii + 1] <- "#if defined COMPILER_MSVC || defined WIN32 || defined __WIN32__"
writeLines(content, file)
##################################################

# Tweak to avoid a compiler error: Execute from shell as R complains about
# unrecognized escape in character string
system("sed -i 's/\* children\[0\]/** children/g' ../inst/include/s2/s2regioncoverer.h")

### GIT: Change drem() to remainder() in geometry C++ source code ###
## (NO LONGER NEEDED AS IT IS DONE UPSTREAM)
# system("git grep -l 'drem' | xargs sed -i 's/drem/remainder/g'")
#####################################################################

#### GIT: Fix nul character at end of string ####
# file <- file.path("base", "logging.cc")
# content <- readLines(file)
# ii <- grep('snprintf', content)
# content[ii] <- '  snprintf(buffer_, sizeof(buffer_), "%02d:%02d:%02d",'
# writeLines(content, file)
#################################################

#### GIT: Fix isnan and isinf problem on Windows ####
# file <- file.path("util", "math", "exactfloat", "exactfloat.cc")
# content <- readLines(file)
# content <- gsub("isnan", "std::isnan", content)
# content <- gsub("isinf", "std::isinf", content)
# writeLines(content, file)
######################################################

# ########### GIT: hash_map and hash_set ##############
# system("git grep -l 'namespace __gnu_cxx' | xargs sed -i 's/namespace __gnu_cxx/namespace std/g'")
# files <- system("git grep -l '<hash_...>'", intern = TRUE)
# remove_chunk <- function(file){
#   content <- readLines(file)
#   ii <- grep("<hash_...>", content)
#   # Lines to delete
#   ii <- rep(ii, each = 5) + rep((-3):1, length(ii))
#   writeLines(content[-ii], file)
# }
# lapply(files, remove_chunk)
# 
# files <- system("git grep -l 'using __gnu_cxx::hash_...;'", intern = TRUE)
# change_chunk <- function(file, type){
#   content <- readLines(file)
#   ii <- grep(paste0("using __gnu_cxx::hash_", type, ";"), content)
#   if(length(ii)>0){
#     content <- append(content,
#                       c(paste0('#include <unordered_', type, '>'), paste0("using std::unordered_", type, ";")),
#                       ii)
#     writeLines(content[-ii], file)
#   }
# }
# lapply(files, change_chunk, type = "map")
# lapply(files, change_chunk, type = "set")
# 
# system("git grep -l 'hash_map<' | xargs sed -i 's/hash_map</unordered_map</g'")
# system("git grep -l 'hash_set<' | xargs sed -i 's/hash_set</unordered_set</g'")
# 
# ## Tweak util/hash/hash.h
# file <- file.path("util", "hash", "hash.h")
# content <- readLines(file)
# ii <- grep('__gnu_cxx::hash', content)
# content <- content[-ii]
# ii <- grep('ext/hash', content)
# content <- content[-ii]
# i1 <- grep('--------- STL hashers ---------', content)
# i2 <- grep('------- Fingerprints --------', content)
# i2 <- i2 - 1
# writeLines(content[-(i1:i2)], file)
# ###################################################

setwd(owd)
