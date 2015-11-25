# python-teng

Python module for template engine [TENG](https://github.com/burlog/teng) supporting both 2 and 3 pytnon versions. It should be compatible with original python module.

## Usage

```python
from teng import Teng

teng = Teng("/some/template/root/directory/")
root = teng.createDataRoot({})
```

The code above creates new instance of the engine and creates fragments root.

```python
frag = root.addFragment("fruit", {"name": "apple"})
frag.addVariable("color", "red")
frag.addVariable("price", 3)
```

Now you can add new fragment or update existing fragments with new values.

```python
root.addFragment("fruit", [{"name": "orange", "price": 3.3}, {"name": "banana"}])

print(root)
```

But you can also add multiple fragments as an array of dicts.

```python
result = teng.generatePage(templateString="<?teng debug?>", data=root, configFilename="/some/conf.conf")

print(result["output"])
```

And at the end you can generate page like above.
