
export const environment = {
  production: true,
  api_service: window.location.origin,
  WS_ENDPOINT: 'wss://' + window.location.hostname + ':8081/',

  firebase: {
    apiKey: 'AIzaSyCwmwA7ukI09QaERBj2l_8lGFVHltANl9c',
    authDomain: 'alpha-trading-demo.firebaseapp.com',
    projectId: 'alpha-trading-demo',
    storageBucket: 'alpha-trading-demo.appspot.com',
    messagingSenderId: '330854289897',
    appId: '1:330854289897:web:85d48f8dc5cb2a27857b78',
  },
};
