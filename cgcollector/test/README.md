# Testing

## CGSimpleTester

The simple tests check whether the `cgcollector`'s generated `.json` files match a pre-generated file that was checked for its validity.
It relies on the `==` operator defined by the used `nlohnmann::json` library for its json objects.
