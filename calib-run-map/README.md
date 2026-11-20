# Calibration run map

A calibration run map tells, for each parameter of one detector, which
runs are analyzed to make it and for which runs it is used.

| file | content |
|---|---|
| `<detector>.toml` | map of one detector (`RF.toml`, `T0.toml`, `TOF.toml`, `SDC.toml`, ...) |
| `example.toml` | example to copy; its values are for testing, not real ones |

The maps are read only through `script/calib_run_map.py`
(see the [top README](../README.md#calibration-run-map)).

## Make or change a map

1. To make the map of a new detector, copy `example.toml` to
   `<detector>.toml` and replace the values.
2. Write the entries as described below.
3. Check the map:
   `python3 script/calib_run_map.py calib-run-map/<detector>.toml`

## Format

A map has one table for each parameter. One line is one entry: one
parameter to make.

```toml
HDPRM = [
  { valid_runs = [7000, 7013], filename_run = 7000, input_runs = [7000, 7000], pedestal_run = 7000 },
  { valid_runs = [7014, -1], filename_run = 7014, input_runs = [7014, 7014], pedestal_run = 7014 },
]

HDPHC = [
  { valid_runs = [7000, 7111], filename_run = 7000, input_runs = [7000, 7013] },
]
```

| key | needed | meaning |
|---|---|---|
| `valid_runs = [A, B]` | always | the parameter is used for runs A to B |
| `filename_run = N` | always | run number used in the name of the output file |
| `input_runs = [X, Y]` | always | runs X to Y are analyzed together to make the parameter |
| `pedestal_run = P` | only where the macro needs it | run used for pedestals; write the run of `input_runs` if the pedestal events are in that run |

The parameter names (`HDPRM`, `HDPHC`, ...) that can be written are
listed in `PARAM_NAMES` of `script/calib_run_map.py`.

## Rules

- A range `[first, last]` includes both first and last.
- A run number is 0 to 99999.
- Keep one entry on one line.
- Order the entries of a parameter by `valid_runs`; `valid_runs` of two
  entries must not overlap.
- Runs that no `valid_runs` covers have no parameter in the map: looking
  them up is an error.

## valid_runs whose end is not decided yet

Write `-1` as the end: `valid_runs = [7014, -1]`.

- Only the last entry of a parameter can have it. When you add the next
  entry, replace `-1` with the last run.
- `-1` is the only way to write it. A large number such as `99999` is an
  error.

## Parameters that are always made together

Write the entries once, and let the other parameter point to them with
`same_as`:

```toml
DCTDC = [
  { valid_runs = [7000, -1], filename_run = 7162, input_runs = [7162, 7162] },
]

DCDRFT = { same_as = "DCTDC" }
```

- `DCDRFT` takes all the entries of `DCTDC` as they are; nothing can be
  changed for `DCDRFT` only.
- The parameter that `same_as` points to must be in the same file and
  must have a list of entries.

## Check a map

```
python3 script/calib_run_map.py calib-run-map/TOF.toml                    # check the map and print it
python3 script/calib_run_map.py calib-run-map/TOF.toml HDPHC              # only HDPHC
python3 script/calib_run_map.py calib-run-map/TOF.toml HDPHC --run 7005   # the entry used for run 7005
```

If the map is correct, the command prints its entries. If not, it
prints one line that tells the file, the parameter and the number of
the entry, and ends with exit code 1.
