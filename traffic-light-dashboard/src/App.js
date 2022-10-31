import logo from './logo.svg';
import './App.css';
import TrafficLightComponent from "./TrafficLight.js";

function App() {
  return (
    <div className="App">
      <header className="App-header">
        <div class="traffic-light-control-group" >      
            <TrafficLightComponent lightvalues={["red", "yellow", "green", "off"]} 
                                    name="Light 1" serial="0001" />
            <TrafficLightComponent lightvalues={["red", "yellow", "green", "off"]} 
                                    name="Light 2" serial="0002" />
            <TrafficLightComponent lightvalues={["red", "yellow", "green", "off"]} 
                                    name="Light 3" serial="0003" />
            <TrafficLightComponent lightvalues={["red", "yellow", "green", "off"]} 
                                    name="Light 4" serial="0004" />
        </div>
        </header>
    </div>
  );
}

export default App;
