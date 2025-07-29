// Use to run the C++ trading engine with PM2
module.exports = {
  apps: [
    {
      name: "cpp_trading_engine",
      script: "/home/ubuntu/cpp_trading_engine/run_bash.sh",
      interpreter: "bash",
      env: {
        PROD: "true"
      }
    }
  ]
};
