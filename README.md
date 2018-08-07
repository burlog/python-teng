# python-teng

Python module for template engine [TENG](https://github.com/burlog/teng) supporting both 2 and 3 python versions. It should be compatible with original python module and can be used in both 2 and 3 python versions.

For now there are theese issues that can be fixed in the future:
 * other charsets than UTF-8 are not supported

## Usage

The code bellow creates new instance of the engine and creates fragments root.

```python
from teng import Teng

teng = Teng("/some/template/root/directory/")
root = teng.createDataRoot({})
```
Now you can add new fragment or update existing fragments with new values.

```python
frag = root.addFragment("fruit", {"name": "banana"})
frag.addVariable("price", 3)
frag.addVariable("weight", "120g")
```

In the added fragment you can add another nested fragment.

```python
frag.addFragment("color", {"name": "yellow", "img": "yellow-banana.jpg"})

#print(root)
````

But you can also add multiple fragments as an array of dicts.

```python
frag = root.addFragment("fruit", {"name": "apple", "color": [
    {"name": "red", "img": "red-apple.jpg"},
    {"name": "green", "img": "green-apple.jpg"}]})
    
#print(root)
```

And at the end you can generate page like above.

```python
result = teng.generatePage(templateString="<?teng debug?>", data=root, configFilename="/some/conf.conf")

print(result["output"])
```

If the */some/conf.conf* exists and contains `%enable debug` than you should see something like this:

```
Template sources:

Language dictionary sources:

Configuration dictionary sources:

Configuration: 
    debug: enabled
    errorfragment: disabled
    logtooutput: disabled
    bytecode: disabled
    watchfiles: enabled
    maxincludedepth: 10
    maxdebugvallength: 40
    format: enabled
    alwaysescape: enabled
    shorttag: disabled

Application data:
fruit[0]: 
    name: "banana"
    price: "3"
    weight: "120g"
    color[0]: 
        img: "yellow-banana.jpg"
        name: "yellow"

fruit[1]: 
    name: "apple"
    color[0]: 
        img: "red-apple.jpg"
        name: "red"

    color[1]: 
        img: "green-apple.jpg"
        name: "green"
```
