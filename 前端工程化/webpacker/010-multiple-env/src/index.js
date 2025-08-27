import helloWorld from "./helloword";
import imgsrc from '../assets/img-1.png'
import logoSvg from '../assets/webpack-logo.svg'
import exampleTxt from '../assets/example.txt'
import jpgMap from "../assets/qianfeng-sem.jpg"
import './style.css'
import './style.less'
import Data from '../assets/data.xml'
import Notes from '../assets/data.csv'

import toml from '../assets/data.toml'
import yaml from '../assets/data.yaml'
import json5 from '../assets/data.json5'
import _ from "lodash"
import './async-module.js'

helloWorld()

const img = document.createElement('img')
img.src = imgsrc
document.body.appendChild(img)

const img2 = document.createElement('img')
img2.style.cssText = 'width: 600px; height: 200px'
img2.src = logoSvg
document.body.appendChild(img2)

const block = document.createElement('div');
block.style.cssText = 'border: 2px solid black; width: 200px; height: 200px; background-color: lightblue; padding: 20px';
block.classList.add('block-bg')
block.textContent = exampleTxt;
document.body.appendChild(block);

const img3 = document.createElement('img')
img3.style.cssText = 'width: 600px; height: 200px;display: block'
img3.src = jpgMap
document.body.appendChild(img3)

document.body.classList.add('hello')

const span = document.createElement('span')
span.classList.add('icon')
span.innerHTML = '&#xe668;'
document.body.appendChild(span)

console.log(Data)
console.log(Notes)
console.log(toml)
console.log(toml.title)
console.log(toml.owner)
console.log(yaml.title)
console.log(yaml.owner)
console.log(json5.title)
console.log(json5.owner)
console.log(_.join(['a', 'b', 'c'], '~'))

const button = document.createElement('button')
button.textContent = 'click me'
button.addEventListener('click', () => {
    import(/* webpackChunkName: 'math',webpackPrefetch:true */'./math.js').then(({add}) => {
        console.log(add(1, 2))
    })
})
document.body.appendChild(button)