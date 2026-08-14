window.BENCHMARK_DATA = {
  "lastUpdate": 1786704024375,
  "repoUrl": "https://github.com/bokic/textparser",
  "entries": {
    "textparser SQLite C/H Parse Benchmark": [
      {
        "commit": {
          "author": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "committer": {
            "email": "bbarbulovski@gmail.com",
            "name": "Boris Barbulovski",
            "username": "bokic"
          },
          "distinct": true,
          "id": "4bef6586da8bd00042d79767171f165a1e57be11",
          "message": "Update benchmark workflow.",
          "timestamp": "2026-08-14T12:38:39+02:00",
          "tree_id": "d7a8eea7005000fb5de38be572ae4aebd94c05c4",
          "url": "https://github.com/bokic/textparser/commit/4bef6586da8bd00042d79767171f165a1e57be11"
        },
        "date": 1786704023704,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2159.072003111111,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2158.5843795555556 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2157.9941666666687,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2157.6402123333332 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.242206235784513,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.9680493887070427 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.001501666563742507,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0013749980852349876 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 55.185788444444164,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 55.18024022222158 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 55.23555100000029,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 55.229296333332435 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.14834944199037609,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.1437564471212843 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0026881819789477193,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0026052160436842806 ms\nthreads: 1"
          }
        ]
      }
    ]
  }
}