/**
 * System configuration for Angular 2 samples
 * Adjust as necessary for your application needs.
 */
(function(global) {
  // map tells the System loader where to look for things
  System.config({
    paths: {
      // paths serve as alias
      'npm:': 'node_modules/'
    },
    map: {
      'app':                        'tsbuild/app', // 'dist',
      '@angular/core': 'npm:@angular/core/bundles/core.umd.js',
      '@angular/common': 'npm:@angular/common/bundles/common.umd.js',
      '@angular/compiler': 'npm:@angular/compiler/bundles/compiler.umd.js',
      '@angular/platform-browser': 'npm:@angular/platform-browser/bundles/platform-browser.umd.js',
      '@angular/platform-browser-dynamic': 'npm:@angular/platform-browser-dynamic/bundles/platform-browser-dynamic.umd.js',
      '@angular/http': 'npm:@angular/http/bundles/http.umd.js',
      '@angular/router': 'npm:@angular/router/bundles/router.umd.js',
      '@angular/forms': 'npm:@angular/forms/bundles/forms.umd.js',

      'rxjs':                       'npm:rxjs',
      'angular2-in-memory-web-api': 'npm:angular2-in-memory-web-api',
      '@ng-bootstrap':              'npm:@ng-bootstrap',
      'angular2-cookie':            'npm:angular2-cookie',
      'moment':                     'npm:moment',
      'ng2-bootstrap':              'npm:ng2-bootstrap'
    },
  // packages tells the System loader how to load when no filename and/or no extension
    packages: {
      'app':                        { main: 'main.js',  defaultExtension: 'js' },
      'rxjs':                       { defaultExtension: 'js' },
      'angular2-in-memory-web-api': { main: 'index.js', defaultExtension: 'js' },
      '@ng-bootstrap': { main: 'index.js', defaultExtension: 'js'},
      'angular2-cookie': { main: './core.js', defaultExtension: 'js'},
      'moment': { main: './moment.js', defaultExtension: 'js'},
      'ng2-bootstrap' : { main: 'index.js', defaultExtension: 'js' },
    }
  });
})(this);
