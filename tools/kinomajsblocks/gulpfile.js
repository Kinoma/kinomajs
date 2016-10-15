const gulp = require('gulp');
const sysBuilder = require('systemjs-builder');
const concat = require('gulp-concat');
const uglify = require('gulp-uglify');
const sourcemaps = require('gulp-sourcemaps');
gulp.task('bundle:libs', function () {
    gulp.src([
        'node_modules/core-js/client/shim.min.js',
        'node_modules/zone.js/dist/zone.js',
        'node_modules/reflect-metadata/Reflect.js',
        'node_modules/systemjs/dist/system.src.js',
        'node_modules/ng2-bootstrap/bundles/ng2-bootstrap.min.js',
        'node_modules/google-code-prettify/bin/prettify.min.js',
        'bower_components/google-blockly/blockly_compressed.js',
        'bower_components/google-blockly/blocks_compressed.js',
        'bower_components/google-blockly/javascript_compressed.js',
        'bower_components/google-blockly/msg/messages.js',
        'bower_components/google-blockly/msg/js/en.js',
        'blocks/kinomablocks.js',
        'blocks/kinomablocksgen.js'
      ])
        .pipe(concat('vendors.min.js'))
        .pipe(gulp.dest('tsbuild/lib/js'));
      gulp.src([
        'node_modules/bootstrap/dist/css/bootstrap.min.css',
        'node_modules/google-code-prettify/bin/prettify.min.css',
        'node_modules/font-awesome/css/font-awesome.min.css'
      ])
        .pipe(concat('vendors.min.css'))
        .pipe(gulp.dest('tsbuild/lib/css'));
      gulp.src([
        'node_modules/font-awesome/fonts/*'
      ]).pipe(gulp.dest('tsbuild/lib/fonts'));
});


// Generate systemjs-based builds
gulp.task('bundle:js', function() {
  var builder = new sysBuilder('', './systemjs.config.js');
  return builder.buildStatic('app', 'tsbuild/dist/js/app.min.js');
});

// Minify JS bundle
gulp.task('minify:js', function() {
  return gulp
    .src('tsbuild/dist/js/app.min.js')
    .pipe(gulp.dest('tsbuild/dist/js'));
});

gulp.task('scripts', [ 'bundle:libs', 'bundle:js', 'minify:js']);
