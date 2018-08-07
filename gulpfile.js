var gulp = require('gulp');
var exec = require('child-process-promise').exec;

var buildCommand = "g++ -fopenmp -std=c++11 -mbmi2 -Wall -Wpedantic -Wextra -mavx2 -DHAVE_AVX2_INSTRUCTIONS -march=haswell -O3 -DNDEBUG src/test.cpp -o build/test";

gulp.task('build', function () {
    return exec(buildCommand)
        .then((result) => {
            console.log(result.stdout);
            console.log(result.stderr);
        })
        .catch((err) => {
            console.log(err);
        });
});

gulp.task('default', ['build'], function () {});