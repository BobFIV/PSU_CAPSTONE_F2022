# Traffic Light Dashboard
This is a simple React JS single page application (SPA) for controlling the Thingy:91 traffic lights.

## Development
To launch the development server, run the following command from this directory: `npm start`

## Deployment
To build a deployment version of this app, run this command: `npm build`.

This will create a set of static files in the `build` directory.  
You can copy/move these files into your `/var/www/html` directory to serve them to your users.

Example workflow:  
```
npm build
sudo cp -r build/* /var/www/html
sudo apachectl start
```
