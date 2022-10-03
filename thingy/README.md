## Linux based thingy:91 dev notes
---

Follow instructions on Nordic's website for downloading nRF connect, VS Code etc.

It seems that it is not possible to build/flash/create a project within VS Code. Must be done from command line.

### Creating a new project

Creating a new project can be started in VS Code but eventually moves to CLI. First click "Create a new application" define the variables it prompts you for (some of these variables it's going to say it can't find what you are pointing to, but this is the only way that it works. Make sure to define an application location. Click create application. From here, we need to work out of the terminal. Navigate to the correct location of the project, you need to copy over the nRF repo into this new repo by calling 'west init' then 'west update' then 'west build -b BOARD_NAME'

