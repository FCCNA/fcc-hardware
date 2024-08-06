import midas.client

"""
A simple example program that demonstrates reading data from the history
system. It emulates a simplified version of the mhist program.
```
"""

if __name__ == "__main__":
    client = midas.client.MidasClient("pytest")
    
    # Read valid history "event" names
    events = client.hist_get_events()

    print("Valid events are:")
    for ev in events:
        print("* %s" % ev)

    event_name = input("Enter event name: ")

    # Read valid history "tag" names for this event
    tags = client.hist_get_tags(event_name)

    print("Valid tags for %s are:" % event_name)
    for tag in tags:
        print("* %s" % tag["name"])

    tag_name = input("Enter tag name: ")

    # Find how many elements are in this event/tag combination
    idx = 0
    n_vars = 1
    for tag in tags:
        if tag["name"] == tag_name:
            n_vars = tag["n_data"]

    print("Event/tag %s/%s has %d elements" % (event_name, tag_name, n_vars))

    if n_vars > 1:
        # If event/tag combination is an array, ask which element to show
        idx = int(input("Element index (0-%d): " % (n_vars-1)))

    num_hours = float(input("How many hours: "))
    interval_secs = int(input("Interval in seconds: "))
    timestamps_as_datetime = True

    # Get the actual data.
    # There is also a `hist_get_data()` function for specifying an arbitrary time period.
    # You can also pass None for the index parameter to get data for all elements in the
    # array.
    data = client.hist_get_recent_data(num_hours, interval_secs, event_name, tag_name, idx, timestamps_as_datetime)

    # Data is a list of dictionaries. In this case list length will be 1, as we
    # only specified one index to search for (rather than all of them).
    num_entries = data[0]["num_entries"]
    timestamps = data[0]["timestamps"]
    values = data[0]["values"]

    print("%d entries found" % num_entries)

    for i in range(num_entries):
        time_string = timestamps[i].strftime("%Y/%m/%d %H:%M:%S")
        print("%s => %f" % (time_string, values[i]))
    
    # Disconnect from midas before we exit
    client.disconnect()