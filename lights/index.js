const express = require('express');
const fs = require('fs');
const cors = require('cors');

const app = express();
const PORT = process.env.PORT || 8080;

app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(cors());

app.post('/api', (req, res) => {
    const { body } = req;
    const { light } = body;

    fs.writeFile('light.json', `{"light1":"${light}"}`, (err) => {
        if (err) {
            console.log(err);
            res.status(500).send('Error');
        }
    });

    res.status(200).send('Success');
})

app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
})

app.get('/state', (req, res) =>{
    const { body } = req;
    fs.readFile('./light.json', (err, lightstring) => {
        if (err) {
            console.log(err);
            res.status(500).send('Error');
        }
        const data = JSON.parse(lightstring);
        res.status(200).send(data.light1);

        // try{
        //     const lightStatus = JSON.parse(lightstring);
        //     console.log(lightStatus.light1);
        // } catch (err) {
        //     console.log("Error parsing JSON string:", err);
        // }
    });
})
