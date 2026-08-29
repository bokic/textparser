window.BENCHMARK_DATA = {
  "lastUpdate": 1787975759059,
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
          "id": "fdcf7372312db4c50a2b2dddd6f4ea47b12ef578",
          "message": "Check if ccontext_create() fails.",
          "timestamp": "2026-08-14T22:36:27+02:00",
          "tree_id": "0fdb2102430f88018578885b4d452e25bbb6a593",
          "url": "https://github.com/bokic/textparser/commit/fdcf7372312db4c50a2b2dddd6f4ea47b12ef578"
        },
        "date": 1786741004065,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2890.495099444444,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2889.989317666667 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2891.1656026666606,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2890.6583069999997 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.1137370242745575,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.9140066947852645 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0007312716166447816,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0006622885015819381 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 75.25297655555757,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 75.24218888888849 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 75.18075033333578,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 75.17604499999919 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.2202834761132444,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.2123729275683972 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.002927239375715778,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0028225245796877625 ms\nthreads: 1"
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
          "id": "673239793845eacb636c67b5855c94fcdbd6d59d",
          "message": "Document API functions.",
          "timestamp": "2026-08-15T10:25:59+02:00",
          "tree_id": "470920f5d6e73ea629dcaf316052044010352586",
          "url": "https://github.com/bokic/textparser/commit/673239793845eacb636c67b5855c94fcdbd6d59d"
        },
        "date": 1786782473932,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2915.1005441111106,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2914.5184111111107 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2912.9561653333367,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2912.4520406666657 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 7.113260091723409,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 7.046162606741102 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.002440142281230449,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0024176078558566633 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 75.86503044444252,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 75.84209666666631 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 75.93711399999847,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 75.90526200000032 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.22472985201128773,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.22401854537171464 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0029622324105684225,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0029537493716227125 ms\nthreads: 1"
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
          "id": "1b9f52cfb77426d02d97e4af79d9055a0736ca61",
          "message": "Add support for C++.",
          "timestamp": "2026-08-15T10:50:04+02:00",
          "tree_id": "367ef8dfc454a223d7dfaa74d406bfc0d87b7035",
          "url": "https://github.com/bokic/textparser/commit/1b9f52cfb77426d02d97e4af79d9055a0736ca61"
        },
        "date": 1786784032944,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2929.0692875555555,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2928.5008614444446 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2928.7004860000015,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2928.104403 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 1.044525999972737,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.0230834872712868 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0003566067912461171,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00034935399908561556 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 76.1633715555566,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 76.14332799999977 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 76.0978630000011,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 76.06655633333285 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.15591521700392616,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.17288069768617695 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0020471154810970437,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.002270464165766139 ms\nthreads: 1"
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
          "id": "80aa56386b1c7921f6ee0963560ebb9726ea57de",
          "message": "Implement error codes/string.",
          "timestamp": "2026-08-15T16:59:28+02:00",
          "tree_id": "651677fe15e1e782fde2a2c8b6a8d2124267cede",
          "url": "https://github.com/bokic/textparser/commit/80aa56386b1c7921f6ee0963560ebb9726ea57de"
        },
        "date": 1786806071502,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2724.3841629999965,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2723.883919444444 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2722.790863999999,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2722.2872826666667 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 6.9638883082967284,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 6.882191280961666 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0025561330163615175,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.002526609607638984 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 70.51427955555785,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 70.50837377777775 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 70.40286900000108,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 70.40215066666633 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.35292911420826867,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.3557313258082289 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.005005072964408543,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.005045235150783543 ms\nthreads: 1"
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
          "id": "00a0f2229bb6510eff6bb70f7c481ba9b2367291",
          "message": "More work on imcremental parser.",
          "timestamp": "2026-08-17T23:18:45+02:00",
          "tree_id": "b229ec3613da0463ca756e94f59a3905084eae7d",
          "url": "https://github.com/bokic/textparser/commit/00a0f2229bb6510eff6bb70f7c481ba9b2367291"
        },
        "date": 1787001634081,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2116.564842666667,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2116.257029111111 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2114.4996136666664,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2114.210172333333 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 6.068104486664254,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 6.105098101772548 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.002866958934751571,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0028848566208126737 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 54.98158477777634,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 54.97759066666674 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 54.851309000000015,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 54.84642433333301 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.3281762286997174,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.32532298880240657 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.005968839021758552,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.005917374422150732 ms\nthreads: 1"
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
          "id": "b17353aa8c610a30cb7fdb21ade40ee300648459",
          "message": "Add FUTURE_WORK.md",
          "timestamp": "2026-08-18T14:05:27+02:00",
          "tree_id": "8517a068ed22d5a7a93343d45c68d3e214a50b09",
          "url": "https://github.com/bokic/textparser/commit/b17353aa8c610a30cb7fdb21ade40ee300648459"
        },
        "date": 1787054826840,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2642.9564561111106,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2642.656113888889 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2643.2107193333345,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2642.899526666667 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 9.789127738177031,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 9.75029720130587 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.003703855095887926,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.003689582291869786 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 68.92696800000111,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 68.92405755555546 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 68.87438566666522,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 68.86827433333309 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.28152682095333603,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.28404484620419734 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.004084421948653414,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004121127749556041 ms\nthreads: 1"
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
          "id": "e5bc82de1607f285be205d0c2ac9ba75b01dc6b2",
          "message": "Remove position from token struct.",
          "timestamp": "2026-08-21T01:10:05+02:00",
          "tree_id": "27ba2c98f6fbed2979402666bc8209f0605884a9",
          "url": "https://github.com/bokic/textparser/commit/e5bc82de1607f285be205d0c2ac9ba75b01dc6b2"
        },
        "date": 1787267510115,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2691.105822777776,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2690.724533333333 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2692.4058563333233,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2692.012393 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.9618648138968324,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 3.9471966956981333 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.001472206994003443,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.001466964249516933 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 61.35778811110665,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 61.253555333333765 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 61.22724466666796,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 61.2042456666669 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 1.335926074089234,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.1955364062720546 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.021772722179458942,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.019517828798117975 ms\nthreads: 1"
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
          "id": "51e782b8784aee15cff6e72474404099f95be651",
          "message": "Update incremental parsing algorithm",
          "timestamp": "2026-08-21T01:41:56+02:00",
          "tree_id": "a9bf680dcf945d7fcadac4fd7c91e18ae92ef858",
          "url": "https://github.com/bokic/textparser/commit/51e782b8784aee15cff6e72474404099f95be651"
        },
        "date": 1787269422102,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 2704.5186478888877,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2704.0368563333327 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 2698.5129403333303,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2697.9955639999994 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 12.907461369971676,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 12.908071213252198 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0047725540291789345,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004773629909303643 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 60.64040911111116,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 60.6196991111112 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 60.55508600000318,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 60.545927666666444 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.20097918244244647,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.20591527935728704 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0033142781420585927,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.003396837700890932 ms\nthreads: 1"
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
          "id": "a91774ef68dfa46fca3fc153d1b88c61a7781517",
          "message": "Performance optimizations.",
          "timestamp": "2026-08-21T06:17:31+02:00",
          "tree_id": "6cb947b01c01ef27309bbec116558e6af90f8e76",
          "url": "https://github.com/bokic/textparser/commit/a91774ef68dfa46fca3fc153d1b88c61a7781517"
        },
        "date": 1787285946509,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1555.7098695555521,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1555.3424804444446 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1554.4302896666597,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1554.2191130000003 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.269265032066419,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.991823452905652 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0014586685322724871,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.001280633351142368 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 42.03740788888884,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 42.03406277777786 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 41.93688533333292,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 41.9372220000002 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.32773118548481084,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.3259834616834619 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.007796179687174181,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.007755221364321687 ms\nthreads: 1"
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
          "id": "ae97f90328a27a2c9e3eef708e596b66ecdb6ed6",
          "message": "Update FUTURE_WORK.md",
          "timestamp": "2026-08-21T08:39:11+02:00",
          "tree_id": "e5630946a67f1a6397a8d39395d2a82053149fec",
          "url": "https://github.com/bokic/textparser/commit/ae97f90328a27a2c9e3eef708e596b66ecdb6ed6"
        },
        "date": 1787294438322,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1561.4868133333332,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1560.6299569999999 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1557.389377333332,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1556.997939333333 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 9.10192502333642,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 8.273887459657848 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.005829011776222677,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.005301633114593513 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 42.022003111112674,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 42.01960888888894 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 41.99110566667249,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 41.99058600000013 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.06533092061223085,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.06191827156234022 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0015546836365578716,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.001473556589402549 ms\nthreads: 1"
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
          "id": "ec8eb46194826da3e0f1d805d4003645fe05e2fb",
          "message": "Implement AST Post-Processing with Pratt Parsing.",
          "timestamp": "2026-08-21T08:57:35+02:00",
          "tree_id": "fd2f998cd3b5610585a50ddc5eb44ba1cb64050e",
          "url": "https://github.com/bokic/textparser/commit/ec8eb46194826da3e0f1d805d4003645fe05e2fb"
        },
        "date": 1787295556093,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1252.5638258888894,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1252.424081111111 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1246.8936403333307,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1246.7601260000001 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 15.246085734487401,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 15.233196604714188 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.012171903275003112,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0121629700629836 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 34.900370111110924,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 34.89435633333334 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 35.17052466666826,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 35.17132300000014 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.678309289402319,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.6779306192472742 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.01943559014539997,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.01942808781945266 ms\nthreads: 1"
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
          "id": "9c537f2f0d7b981f1280c543b7339918a5049383",
          "message": "Update tests.",
          "timestamp": "2026-08-21T09:20:39+02:00",
          "tree_id": "f6a7d0d0bc279bc9927d4d7320ca1bd83f422efe",
          "url": "https://github.com/bokic/textparser/commit/9c537f2f0d7b981f1280c543b7339918a5049383"
        },
        "date": 1787296953302,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1669.5456188888882,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1669.3176610000003 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1670.5900969999968,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1670.2946103333334 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.1995167877853983,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 3.153751344119691 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0019163997386994031,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0018892457785598727 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 45.217800888888995,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 45.204880555555604 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 45.20567166666467,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 45.20591033333332 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.41437132773230867,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.3946130077378082 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.009163898278700431,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.008729433700258077 ms\nthreads: 1"
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
          "id": "dbfd9858519b06c8805c9e0d81e129e19850a8f2",
          "message": "Enable Windows CI unittests.",
          "timestamp": "2026-08-21T09:33:00+02:00",
          "tree_id": "ca60bed5e279fd35322f824ba2b4437983bf6089",
          "url": "https://github.com/bokic/textparser/commit/dbfd9858519b06c8805c9e0d81e129e19850a8f2"
        },
        "date": 1787297726291,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1647.7021675555552,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1647.4923282222219 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1649.2013810000024,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1649.015185 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.1578957677375996,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 3.148770696344054 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0019165452530917578,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0019112505972891743 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 44.898403222223095,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 44.88456755555583 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 44.78775900000187,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 44.774897333333875 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.4525333410606837,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.4477860941393545 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.01007905200594519,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.009976393190936008 ms\nthreads: 1"
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
          "id": "c04bc93bb678990d0308a7ed159520e94157aa0b",
          "message": "Implement Contextual Rule Disambiguation.",
          "timestamp": "2026-08-21T10:01:50+02:00",
          "tree_id": "5e077313f1f899869d9bbbe862f5c59b97d93c23",
          "url": "https://github.com/bokic/textparser/commit/c04bc93bb678990d0308a7ed159520e94157aa0b"
        },
        "date": 1787299424264,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1159.8325639999985,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1159.626543 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1132.3592810000018,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1132.1562356666668 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 47.90216201602734,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 47.95659530853292 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.04130092868820969,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.041355206637877825 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 31.53277800000056,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 31.525704333333433 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 31.497900000005075,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 31.493321333333196 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.4409091806051764,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.44343055076487436 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.013982566984905949,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.014065682595900544 ms\nthreads: 1"
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
          "id": "14deec7222a008d1732fa796dc455803dff4b23e",
          "message": "Implement Flat Token Range Struct.",
          "timestamp": "2026-08-21T15:34:18+02:00",
          "tree_id": "7ddbdfd6a1cd610c41b02473ab19dc29953caeeb",
          "url": "https://github.com/bokic/textparser/commit/14deec7222a008d1732fa796dc455803dff4b23e"
        },
        "date": 1787319346518,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1807.0583084444454,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1806.760609111111 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1808.5740603333372,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1808.2558593333335 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.7011312880141176,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.698756717116748 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0014947670893582338,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0014936991118289218 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 48.41145855555359,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 48.40624000000006 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 48.40159233333452,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 48.39633033333376 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.037288153234434636,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.03195803557055218 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0007702340385312988,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.000660204873804537 ms\nthreads: 1"
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
          "id": "e02b5818f9b2aea1eb81efb84f9ce49d97d50e4a",
          "message": "Implement Native C Regex Bypass",
          "timestamp": "2026-08-21T17:12:43+02:00",
          "tree_id": "fa27a50362dc754371181b8411665f85007a3fd7",
          "url": "https://github.com/bokic/textparser/commit/e02b5818f9b2aea1eb81efb84f9ce49d97d50e4a"
        },
        "date": 1787325246145,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 770.5605186666686,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 770.4201845555555 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 771.2928423333333,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 771.150411 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 5.69666631526535,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 5.649499459419457 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.007392886317511463,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.007333010703345699 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 20.640517555555167,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 20.64119111111109 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 20.408064666668224,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 20.408726000000026 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.5117638933541064,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.5116794920844469 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.024794140552758127,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.024789242506892507 ms\nthreads: 1"
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
          "id": "3b03cb8d1708d98f9632f6282d477631272b23b3",
          "message": "Fix windows build.",
          "timestamp": "2026-08-21T17:20:51+02:00",
          "tree_id": "ab4fb29d538dafc2d33f37347a415242d6a06595",
          "url": "https://github.com/bokic/textparser/commit/3b03cb8d1708d98f9632f6282d477631272b23b3"
        },
        "date": 1787325725883,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 615.6268526666667,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 615.5540145555555 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 618.3027959999995,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 618.2417409999999 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 4.689784427153515,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.68927746544149 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.007617901017213775,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.007617978852476917 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 15.426112222221553,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 15.425419777777746 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 15.428784000003285,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 15.42913866666673 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.0696241251509172,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.07131875711847196 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.004513394181757772,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004623456485846537 ms\nthreads: 1"
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
          "id": "bb9ea49c5ba2b59cdb188a6e65c13c5f36aed73a",
          "message": "Fix MacOS shared library search.",
          "timestamp": "2026-08-21T17:40:33+02:00",
          "tree_id": "fde0cc19df15685c0deb00095bc951b2db0c0bf3",
          "url": "https://github.com/bokic/textparser/commit/bb9ea49c5ba2b59cdb188a6e65c13c5f36aed73a"
        },
        "date": 1787326925551,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 768.3310211111124,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 768.2470264444445 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 769.3895816666677,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 769.3001926666666 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 1.8372295197732578,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.8295200810389174 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0023911952912123364,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0023814216235970276 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 20.02421900000052,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 20.020055888888873 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 20.028739333336414,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 20.024587666666704 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.09041961719380386,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.09054809289231425 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.004515512799465562,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004522869136572611 ms\nthreads: 1"
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
          "id": "3a2eb0111995f59ff9c5159dae09ea0ef977424d",
          "message": "Update md files.",
          "timestamp": "2026-08-21T19:34:39+02:00",
          "tree_id": "da876b5072e1e68fba2fed8d391875673c217b13",
          "url": "https://github.com/bokic/textparser/commit/3a2eb0111995f59ff9c5159dae09ea0ef977424d"
        },
        "date": 1787333765148,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 782.2813515555539,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 782.0287703333332 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 782.1735273333321,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 781.9947286666667 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.20178218769574216,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.12747404503807003 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.00025794068501633273,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00016300429072927238 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 19.865124555554164,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 19.865030777777704 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 19.809104999997846,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 19.809064333333144 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.0995757837904353,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.09953358407932938 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0050125929747867875,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.005010492316511989 ms\nthreads: 1"
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
          "id": "4c08bc19e9771f204306ae26e464a0d3cad2164f",
          "message": "Cleanup compiler warnings.",
          "timestamp": "2026-08-21T19:47:42+02:00",
          "tree_id": "40e3edb36c6474395d5b212fcf2f9d499216e0d3",
          "url": "https://github.com/bokic/textparser/commit/4c08bc19e9771f204306ae26e464a0d3cad2164f"
        },
        "date": 1787334540796,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 783.2623361111106,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 783.1534771111109 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 782.1511833333356,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 781.9923123333332 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.2786402468158276,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 3.3086592251959046 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0041858775733992805,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004224790314921227 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 20.127953888889905,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 20.12010755555554 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 20.123672333336156,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 20.119882666666495 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.0318926148496081,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.04122412673012598 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0015844936363458174,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0020489019065279912 ms\nthreads: 1"
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
          "id": "fbd7642d344d35d214b1c795f65aac661c389101",
          "message": "Update README.md",
          "timestamp": "2026-08-22T12:36:06+02:00",
          "tree_id": "26020ee2ace69ac3a5ce45712f80e7b7758e6c0d",
          "url": "https://github.com/bokic/textparser/commit/fbd7642d344d35d214b1c795f65aac661c389101"
        },
        "date": 1787395049582,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1102.7882815555552,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1102.659415111111 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1102.5961896666702,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1102.4591506666666 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.7274631188627745,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.7248032116353689 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0006596580060105827,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0006573228339616844 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 35.14285555555495,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 35.14209933333331 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 34.1132773333328,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 34.10960533333333 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.634233358932495,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.635129906432458 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.07495786319265418,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.07498498827396347 ms\nthreads: 1"
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
          "id": "fcfdbaba612ebab03173ca538e05c590f27a4f65",
          "message": "Add support for .md files.",
          "timestamp": "2026-08-22T12:48:47+02:00",
          "tree_id": "7a0e706bede7990b0689e834de68912abaa902ef",
          "url": "https://github.com/bokic/textparser/commit/fcfdbaba612ebab03173ca538e05c590f27a4f65"
        },
        "date": 1787395826581,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1199.8968371111118,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1199.723092 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1198.1186439999997,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1197.9416509999999 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 4.277079710276842,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.287702567696878 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.003564539532060438,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.003573910176680069 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 36.80047144444308,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 36.79754611111107 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 36.742536000000804,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 36.742584666666765 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.4292052023217245,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.43330108211625845 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.01166303543066525,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.011775271122913891 ms\nthreads: 1"
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
          "id": "930ab17f9d9626d4c9ffa99368229be18fc62b64",
          "message": "Update cfml definition.",
          "timestamp": "2026-08-25T07:25:04+02:00",
          "tree_id": "04c0761a2b3261cb3ab237ea20e13bf808c4cacd",
          "url": "https://github.com/bokic/textparser/commit/930ab17f9d9626d4c9ffa99368229be18fc62b64"
        },
        "date": 1787635588302,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1266.2459732222196,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1266.0769349999998 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1265.9361993333391,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1265.748969333333 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.1726362554075203,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.1790983290811794 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.001715809014483029,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00172114211138455 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 38.90846822222076,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 38.905501444444624 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 39.005969666656405,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 39.006658000000506 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.17681596006634137,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.1770071784460551 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.004544408149312883,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004549669632167927 ms\nthreads: 1"
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
          "id": "240698a8caf5c16267c5e9da88826d5c2c7422ee",
          "message": "Remove trace_search",
          "timestamp": "2026-08-25T07:41:11+02:00",
          "tree_id": "71ce7559eff3a38caa62fe4eda635111b755f71e",
          "url": "https://github.com/bokic/textparser/commit/240698a8caf5c16267c5e9da88826d5c2c7422ee"
        },
        "date": 1787636569408,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1693.6935188888892,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1693.4446488888889 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1693.7210200000075,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1693.4233123333336 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 5.678616944916861,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 5.660605325851023 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0033528007762834177,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0033426574228836397 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 51.25470966666512,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 51.25149877777761 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 50.99664166666192,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 50.99636633333364 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.5410843274361854,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.5439364478666759 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.010556772849853721,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.010613083730978108 ms\nthreads: 1"
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
          "id": "d864e60605a6b30b380f1ab3f9f645bf4e3fa32a",
          "message": "Update dynamic lib loading for MacOS",
          "timestamp": "2026-08-25T07:58:20+02:00",
          "tree_id": "f58a9485566c4b79c529c1de67795007a71bf3c4",
          "url": "https://github.com/bokic/textparser/commit/d864e60605a6b30b380f1ab3f9f645bf4e3fa32a"
        },
        "date": 1787637606378,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1472.0734356666644,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1471.8447326666665 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1471.3164716666633,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1471.1224853333335 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.6009283883050265,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.585247987848407 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0017668469013077002,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0017564678735945828 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 45.916013444446605,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 45.91402111111106 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 45.75537466666901,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 45.75127466666669 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.3219508968775782,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.3210852029558224 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.007011734528458222,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.006993184112077708 ms\nthreads: 1"
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
          "id": "3732550ce911034c8b159da73c12258999e79d17",
          "message": "update json2h.py script.",
          "timestamp": "2026-08-25T08:56:15+02:00",
          "tree_id": "c959b214d820ceaa56841bb62db5ce68b61734bb",
          "url": "https://github.com/bokic/textparser/commit/3732550ce911034c8b159da73c12258999e79d17"
        },
        "date": 1787641199942,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1660.246816999999,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1659.9946407777777 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1661.507456,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1661.2285893333335 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 8.475653241750644,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 8.50116356296041 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.005105056160906134,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.005121199402774732 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 50.63546400000026,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 50.63078166666656 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 50.43091200000068,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 50.42158533333326 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.42631701396959093,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.4257147940880861 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.008419336573465363,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00840822085052582 ms\nthreads: 1"
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
          "id": "36cd0775290d3495325de96b8127ad99f1e8c874",
          "message": "Suppress c2y-extensions only where needed.",
          "timestamp": "2026-08-27T03:13:00+02:00",
          "tree_id": "127db15abaa7326359e499e6ed360a6e3ce8656a",
          "url": "https://github.com/bokic/textparser/commit/36cd0775290d3495325de96b8127ad99f1e8c874"
        },
        "date": 1787793289647,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1271.6486298888929,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1271.6272195555555 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1268.535853000003,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1268.5242750000002 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 5.443197443730774,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 5.445938011329012 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.004280425674037301,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004282652909264095 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 39.53863666666595,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 39.46039100000003 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 39.50918766666215,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 39.50298133333341 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.4646998694769109,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.35223300566858895 ms\nthreads: 1"
          },
          {
            "name": "BM_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.011753057481334147,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.008926242156814633 ms\nthreads: 1"
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
          "id": "64712fbaee7937386ed384c1122e436a7983f3d5",
          "message": "Build tree-sitter lib for benchmark binary.",
          "timestamp": "2026-08-27T14:20:19+02:00",
          "tree_id": "c25389eae15addcd0191f56b5f1df1b9ff08c3d4",
          "url": "https://github.com/bokic/textparser/commit/64712fbaee7937386ed384c1122e436a7983f3d5"
        },
        "date": 1787833421379,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1630.1245855555555,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1629.949015777778 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1625.9763453333326,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1625.787121 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 7.977428881301038,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 7.953159305973927 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.004893754104433856,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0048793914588664885 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 2609.244728555555,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2608.9975075555553 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 2605.7792459999973,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2605.4815713333332 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 8.243617264625653,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 8.237009846521229 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.003159388298999924,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0031571551228650736 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1728.8186432222212,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1728.6164871111112 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1727.5173713333345,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1727.215459 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.1485645772384943,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 3.21238331149152 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0018212231743233115,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0018583551270299995 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 48.407911888887156,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 48.40143877777731 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 48.500268999996855,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 48.47908033333207 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.2381991291755091,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.2342165053592773 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.004920665235927924,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004839040145782067 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 76.94786511111336,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 76.93720733333325 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 76.99442600000832,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 76.99168133333482 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.09674571125552578,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.11383429291471943 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.001257289089383106,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.001479574017049101 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 51.410326555554775,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 51.4070022222231 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 51.181045000002236,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 51.18199866666847 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.6612370234822528,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.6588805212349463 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.012861949491172945,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.012816941131613278 ms\nthreads: 1"
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
          "id": "2d302178aaec0d8bb975dd756bffc6abe20a7b17",
          "message": "Patch tree-sitter windows exports.",
          "timestamp": "2026-08-27T14:29:07+02:00",
          "tree_id": "586ea0e70cdf4cd0cd697df339972d6374c0f4af",
          "url": "https://github.com/bokic/textparser/commit/2d302178aaec0d8bb975dd756bffc6abe20a7b17"
        },
        "date": 1787833925094,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1626.5765241111087,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1626.2910275555557 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1628.238092999998,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1627.9118656666667 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 3.5142794566362396,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 3.5008899691668485 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0021605374260253276,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0021526835663780077 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 2624.402455888892,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2623.9959731111107 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 2617.3338193333343,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2616.9154869999993 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 12.568337212859284,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 12.595043372782577 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.004789028140351422,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004799947675929322 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1762.189110111109,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1761.9118095555557 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1752.655911000005,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1752.4023206666661 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 18.869528022436043,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 18.84439271429261 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.010708003990131507,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.010695423353252926 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 48.11814688888679,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 48.10805099999982 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 47.81277300000397,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 47.807682333332956 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 1.045542353360042,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.0469159646555803 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.02172864960436615,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.021761762176887696 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 76.06589822222531,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 76.05421099999887 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 75.90227899999984,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 75.89346866666584 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.34043990711987404,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.3456393301199432 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.004475591757626844,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00454464421595207 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 50.75997533332889,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 50.75525255555548 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 50.598875333330774,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 50.59477199999899 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.42399999044245035,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.42823718292500085 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.008353037755793645,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00843729784333676 ms\nthreads: 1"
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
          "id": "95554e64b5c3ae3bbf4523634017d36327732152",
          "message": "Fix compiler warnings.",
          "timestamp": "2026-08-27T14:54:41+02:00",
          "tree_id": "19c0c8e128c2b3867d41e8f566a37ddb924356b8",
          "url": "https://github.com/bokic/textparser/commit/95554e64b5c3ae3bbf4523634017d36327732152"
        },
        "date": 1787835407440,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1071.8271713333331,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1064.0632242222223 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1061.7091116666634,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1061.5251323333334 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 17.583594073588607,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.39854161509209 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.01640525127919172,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004133722052378237 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 1620.8775468888882,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1620.6117047777777 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 1620.4431186666661,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1620.1379946666668 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 1.6481334189945054,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.6986762448498576 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0010168155035264273,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.001048169798997462 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1220.6213861111116,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1220.515996444444 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1218.4243889999968,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1218.3355053333323 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 4.380001550949356,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.353249071059028 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.00358833754740608,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0035667284031841696 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 31.823753333330586,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 31.82225799999985 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 31.836860999997423,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 31.836808333333504 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.28543664383925643,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.2833764699687776 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.008969295382902072,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.008904976823730702 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 47.536259000000626,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 47.53267000000013 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 47.402830666669615,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 47.400045666667744 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.33866764388321857,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.3378149676088777 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.007124406737249014,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00710700593105493 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 33.88912366666671,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 33.88893000000021 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 33.59605566666346,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 33.59588166666564 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.5335398402225087,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.5335485978986876 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.01574368949372679,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.01574403788784964 ms\nthreads: 1"
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
          "id": "c56680690e0903791c45df722f89a8ea084a8814",
          "message": "Fix: Dynamic PCRE2 State Isolation and Lifecycle Cleanup on textparser_close()",
          "timestamp": "2026-08-29T03:06:01+02:00",
          "tree_id": "2e8bb47b5df9b25e6fb7284f57332c4df01f1447",
          "url": "https://github.com/bokic/textparser/commit/c56680690e0903791c45df722f89a8ea084a8814"
        },
        "date": 1787965718659,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1502.3188835555554,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1502.0582772222222 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1500.9236803333342,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1500.6253800000002 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.6377776321223787,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.6557963414125743 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0017558040846025445,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0017681047278165377 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 2385.2407450000023,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2384.8647872222223 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 2353.469535000002,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2353.114462333334 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 58.469387339551474,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 58.450344715274696 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.024512992016473126,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.024508871542086412 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1714.1670784444425,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1713.8827189999995 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1715.8169796666605,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1715.5361416666656 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 11.044075260265187,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 11.009857292434601 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.006442823105835966,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.006423926894401811 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 44.880636222226634,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 44.87005833333373 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 44.84680866666698,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 44.84626766666603 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.15376549755347035,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.1472838705969647 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0034260988813104147,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.003282453289960363 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 69.05453455554935,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 69.04290433333276 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 68.83263366665915,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 68.81010166666594 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.4787184233766438,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.48715162345134505 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.006932469047221651,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.007055781157458587 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 49.490299333336274,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 49.4858394444443 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 49.19705233334071,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 49.18991133333369 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.648388174418674,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.6470213887596832 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.013101318503885566,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.01307487952156631 ms\nthreads: 1"
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
          "id": "c7bddea72e55e2a152668edb814168c74c91a9f0",
          "message": "Add all languages to textparser cli.",
          "timestamp": "2026-08-29T03:28:41+02:00",
          "tree_id": "9c94a396d8fe4afb7487cfe842dcb9d2fdbb2e31",
          "url": "https://github.com/bokic/textparser/commit/c7bddea72e55e2a152668edb814168c74c91a9f0"
        },
        "date": 1787967169465,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1626.1743776666701,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1625.9923637777777 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1628.0546483333371,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1627.8779996666665 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 4.90655976557836,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.926061009940629 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0030172408524961375,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0030295720445424348 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 2599.263401666666,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2598.9779323333332 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 2597.121833666667,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2596.832316333334 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 4.535544785033506,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.526487276401475 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0017449346542275335,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.001741641289096151 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1724.4065810000013,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1723.8375005555554 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1722.1199076666664,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1721.3042630000002 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 6.951606305730592,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 6.742665329318622 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.00403130351178507,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.003911427455978656 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 49.03006011111364,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 49.02731288888873 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 48.96087066667102,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 48.952617999998914 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.14268963610900862,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.14471102073773398 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0029102480352999846,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.002951640875478829 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 76.29064944444191,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 76.28256677777834 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 76.0241176666625,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 76.02069566666596 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.7017945770805558,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.7007152068615246 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.009198959272087892,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.009185784333959352 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 50.111382555554776,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 50.10350666666675 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 49.87249499999772,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 49.859785000000066 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.5187253927936691,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.5177569117421678 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.010351448440254003,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.010333746002784774 ms\nthreads: 1"
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
          "id": "f42c68bff96e5e39ae5712cbc49c82c6c59dd69b",
          "message": "Update bash definition.",
          "timestamp": "2026-08-29T03:44:49+02:00",
          "tree_id": "92eed5efd44bb712fc4315b2d2c4a8531f8a4442",
          "url": "https://github.com/bokic/textparser/commit/f42c68bff96e5e39ae5712cbc49c82c6c59dd69b"
        },
        "date": 1787968024667,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1531.6783171111074,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1531.486729444444 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1532.916620333329,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1532.7449236666669 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.697848805802486,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.61379468502319 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0017613677595768856,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0017067041031242625 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 2390.4635212222256,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2390.2266941111106 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 2391.140832666669,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2390.8859063333325 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 1.8990750266090453,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.8879003159150003 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0007944379865031627,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0007898415328413368 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1717.1424005555593,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1716.976135555555 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1716.4465526666675,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1716.273115 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 4.1980330925340725,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.179230680391034 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.002444778657364615,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.002434064512515066 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 45.243117333332826,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 45.24283311111156 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 45.226729999995996,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 45.227861999999654 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.04111842964604984,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.04267743353696813 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0009088328141296284,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0009432971059119336 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 70.09640677778053,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 70.09415111111157 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 70.09230633333156,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 70.09371266666638 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.04218971240861077,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.03635264970223456 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0006018812425344497,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0005186260069632516 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 49.047566444446346,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 49.03885477777849 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 49.01789533333082,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 49.00463766666737 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.18866407703268673,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.18594472571973183 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0038465532687819853,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0037917836083723347 ms\nthreads: 1"
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
          "id": "e12d71c508ff3c4cf296087519808c11b144b81e",
          "message": "More bash updates.",
          "timestamp": "2026-08-29T05:04:45+02:00",
          "tree_id": "f09c88375b6dd3f6e3b08c0aa802e3c2d32e3c3f",
          "url": "https://github.com/bokic/textparser/commit/e12d71c508ff3c4cf296087519808c11b144b81e"
        },
        "date": 1787972821127,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1531.954107222221,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1531.7249317777776 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1531.0596393333308,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1530.9538966666667 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.474845172407287,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.400520870961632 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0016154825792364894,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0015672010170752375 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 2366.4350496666675,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2366.219905555556 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 2366.400796333333,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2366.172613666668 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 1.3566966776413951,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1.3250242838209538 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0005733082248897967,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0005599751234912616 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1695.007769,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1694.8841188888894 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1694.5878726666688,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1694.4645246666662 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 4.859156026887786,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.840846174991978 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.002866745578254506,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00285615171033963 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 45.798478777776886,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 45.79818022222228 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 45.716145333329905,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 45.716655666666384 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.16379797772657181,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.16229446955081223 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.00357649385084058,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0035436881719606707 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 69.74113633333128,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 69.73690444444476 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 69.67576766666639,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 69.6721163333341 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.38680771358456406,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.39160584941548326 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.005546335117566726,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.005615475085038401 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 46.25147066666576,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 46.24878944444472 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 46.13045966666126,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 46.124711666666464 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.33466880808152527,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.3371971590194565 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.007235852249834013,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0072909402185435965 ms\nthreads: 1"
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
          "id": "64bd01ad7218ee71186591543a0c2960fd3f159f",
          "message": "Set bash default encoding to utf-8.",
          "timestamp": "2026-08-29T05:16:08+02:00",
          "tree_id": "39520cb9b4d5cfb369ecff4e1e16a1ce40e27f1c",
          "url": "https://github.com/bokic/textparser/commit/64bd01ad7218ee71186591543a0c2960fd3f159f"
        },
        "date": 1787973529622,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1680.9565893333327,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1680.7584396666664 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1680.9801883333319,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1680.7938013333335 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.40260656165938813,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.4015722398386455 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.00023951038605884637,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00023892323272717677 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 2664.3653965555527,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2664.1329 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 2664.808515333334,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2664.5590936666667 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.9993511894557657,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.9814000627596062 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0003750803815226359,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00036837503968349563 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1724.2071936666653,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1724.0409931111117 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1723.7378770000003,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1723.5787186666678 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 4.131760632376882,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.109875692719406 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.002396324900831878,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.002383861932019926 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 49.61511455555486,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 49.60843777777847 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 49.668084333328956,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 49.657765333333735 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.21181865220583038,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.21126702821966084 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0042692363829706275,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.004258691417900192 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 76.70489466666355,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 76.7056897777773 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 76.63285933332986,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 76.63358066666603 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.22985506292429295,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.22987337470283428 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.002996615325829911,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0029968229914729455 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 49.81190444444413,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 49.80498288888905 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 49.72377466665989,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 49.72187166666705 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.4160999129876473,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.4081146361637962 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.008353423094909553,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.008194253114678654 ms\nthreads: 1"
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
          "id": "4be506c323d39c6198ae1fe81b8cfd144597faa7",
          "message": "Update ColdFusion definition.",
          "timestamp": "2026-08-29T05:52:04+02:00",
          "tree_id": "1a515dd2c3b776928192915d3399f7ee67d688d5",
          "url": "https://github.com/bokic/textparser/commit/4be506c323d39c6198ae1fe81b8cfd144597faa7"
        },
        "date": 1787975758420,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1708.43819722222,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1708.2296491111108 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1708.128277999999,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1707.8690720000002 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.4393263113829513,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.3984496588644992 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0014278106842548332,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0014040557486591741 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 2696.6121802222224,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2696.198885111111 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 2694.327516666666,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2694.026093666667 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 4.280946442851192,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 4.040614500968417 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseC_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0015875276668439612,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0014986336962311674 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_mean",
            "value": 1738.1746386666664,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1737.997012 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_median",
            "value": 1739.3610690000023,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 1739.1720343333336 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_stddev",
            "value": 2.14952728122417,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 2.1322793416912833 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseC/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.0012366578325369237,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.0012268601884634787 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 50.894293333338204,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 50.88865944444462 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 50.83395300000385,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 50.824463666666965 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.4859655808048346,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.4870043746299078 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.009548527918875805,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.00956999810854858 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_mean",
            "value": 78.27920955555594,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 78.26908755555529 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_median",
            "value": 78.22222033333522,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 78.21625866666675 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.4066459684274408,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.4114147802343591 ms\nthreads: 1"
          },
          {
            "name": "BM_TextParser_ParseH_JSON/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.005194814443531625,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.005256414672553038 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_mean",
            "value": 50.42945233333285,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 50.424793888888225 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_median",
            "value": 50.109751333328255,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 50.1104769999993 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_stddev",
            "value": 0.5997888763969496,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.5978287101053917 ms\nthreads: 1"
          },
          {
            "name": "BM_TreeSitter_ParseH/min_warmup_time:1.000/iterations:3_cv",
            "value": 0.011893622647979883,
            "unit": "ms/iter",
            "extra": "iterations: 3\ncpu: 0.011855848363460169 ms\nthreads: 1"
          }
        ]
      }
    ]
  }
}