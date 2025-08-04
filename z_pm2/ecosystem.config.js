// Use to run the C++ trading engine with PM2
module.exports = {
  apps: [
    {
      name: "cpp_trading_engine",
      script: "./run_bash.sh",
      interpreter: "bash",
      cwd: "/home/ubuntu/cpp_trading_engine",
      env: {
        PROD: "true",
        LOG_LEVEL: "info",
      }
    }
  ]
};
