
# What do these scripts do?

## How to install midas?

Download midas from the webpage [midas](https://bitbucket.org/tmidas/midas/src/develop/). 
Run the command 
```
pip3 install -e midas/python
```
in the same directory of the downloaded files
Modify the import in the script with the path of your midas/python location

## info-run
`info-run` gives the information of the run: Run Description - Data Taking Time - Number of Events - Size of the run. 
For example
```./info-run 123 112 211 100:112``` 
You are requiring the info of run 123 112 211 and from 100 to 112.

