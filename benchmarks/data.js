window.BENCHMARK_DATA = {
  "lastUpdate": 1786736550170,
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
      },
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
          "id": "8e870314398e740347b6ee27d7cdbc60acdba372",
          "message": "Update Python variant.",
          "timestamp": "2026-08-14T18:36:26+02:00",
          "tree_id": "9f293783cfa55cc5c8b90cb861e44eed6c5c8725",
          "url": "https://github.com/bokic/textparser/commit/8e870314398e740347b6ee27d7cdbc60acdba372"
        },
        "date": 1786725486446,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2986.068665444441,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2985.619074 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2988.2088876666635,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2987.7596206666663 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 5.422024666064785,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 5.522826408687949 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0018157736052120492,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0018498094605514128 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 77.27256433333372,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 77.25216366666663 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 77.28050366666632,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 77.26052166666737 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.07322020741758958,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.05617197348485712 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.000947557623450112,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0007271249220569681 ms\nthreads: 1"
          }
        ]
      },
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
          "id": "b0c7dd24bb2c841dcddec94940242b5777434a51",
          "message": "Add Java implementation.",
          "timestamp": "2026-08-14T19:00:49+02:00",
          "tree_id": "cf40d81b696fa2f2a4161d050c9bd8c35305f53b",
          "url": "https://github.com/bokic/textparser/commit/b0c7dd24bb2c841dcddec94940242b5777434a51"
        },
        "date": 1786726953625,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2997.618347777783,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2997.221619111111 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2997.277549000008,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2996.7650106666665 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.9801131378525265,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.009959808209345 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0013277584655842173,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0013378923275612106 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 77.76245711110934,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 77.75476877777739 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 77.78653000002578,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 77.77449999999912 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.2536585037356118,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.25816496621829604 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.003261966161552442,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0033202460797758884 ms\nthreads: 1"
          }
        ]
      },
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
          "id": "7882dc4cafa4354c6eb99e0f1101bbfcd7588698",
          "message": "Add WebAssembly implementation.",
          "timestamp": "2026-08-14T20:41:12+02:00",
          "tree_id": "c7381af931976ea4d1cf4ad69705b2fd9d17ddd8",
          "url": "https://github.com/bokic/textparser/commit/7882dc4cafa4354c6eb99e0f1101bbfcd7588698"
        },
        "date": 1786732964821,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2529.175467555556,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2528.864329888889 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2530.434223999999,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2529.7492333333334 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 8.792062969762128,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 8.692574228803492 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.003476256622977465,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0034373430500264984 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 65.65701666666573,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 65.65510366666692 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 65.64209833333241,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 65.64314033333336 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.5213812636495828,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.5144373391365111 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.007940983159447903,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00783545086987184 ms\nthreads: 1"
          }
        ]
      },
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
          "id": "225b1866353d0fc60f1d44b004a630cd6dffb794",
          "message": "Update ecintella test project to use incremental parser.",
          "timestamp": "2026-08-14T21:40:46+02:00",
          "tree_id": "5548a0e8b2403c196122c52ace8a127d7b0bb067",
          "url": "https://github.com/bokic/textparser/commit/225b1866353d0fc60f1d44b004a630cd6dffb794"
        },
        "date": 1786736549444,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2688.0572382222217,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2687.556268888889 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2684.384551666663,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2683.994314000001 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 8.2265990704354,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 8.005577215980239 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0030604255569632727,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.002978757062187937 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 69.69055733333738,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 69.6846036666668 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 69.63463533333918,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 69.6281186666674 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.2224205827251771,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.22724630911347998 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.003191545472384669,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.003261069119378257 ms\nthreads: 1"
          }
        ]
      }
    ]
  }
}