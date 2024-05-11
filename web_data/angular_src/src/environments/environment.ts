// This file can be replaced during build by using the `fileReplacements` array.
// `ng build` replaces `environment.ts` with `environment.prod.ts`.
// The list of file replacements can be found in `angular.json`.
const url = 'localhost';

export const environment = {
  production: false,
  api_service: 'https://' + url + ':8080',
  WS_ENDPOINT: 'wss://' + url + ':8081/',

  firebase: {
    apiKey: 'AIzaSyCwmwA7ukI09QaERBj2l_8lGFVHltANl9c',
    authDomain: 'alpha-trading-demo.firebaseapp.com',
    projectId: 'alpha-trading-demo',
    storageBucket: 'alpha-trading-demo.appspot.com',
    messagingSenderId: '330854289897',
    appId: '1:330854289897:web:85d48f8dc5cb2a27857b78',
  },
};

/*
 * For easier debugging in development mode, you can import the following file
 * to ignore zone related error stack frames such as `zone.run`, `zoneDelegate.invokeTask`.
 *
 * This import should be commented out in production mode because it will have a negative impact
 * on performance if an error is thrown.
 */
// import 'zone.js/plugins/zone-error';  // Included with Angular CLI.
