from functools import lru_cache
from pydantic import Field
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    database_url: str = Field("sqlite:///./runtime-data/backend/porchlight.db", alias="PORCHLIGHT_DATABASE_URL")
    device_token: str = Field("dev-device-token", alias="PORCHLIGHT_DEVICE_TOKEN")
    dashboard_origin: str = Field("http://localhost:5173", alias="PORCHLIGHT_DASHBOARD_ORIGIN")
    enable_seed: bool = Field(True, alias="PORCHLIGHT_ENABLE_SEED")

    model_config = SettingsConfigDict(env_file=".env", extra="ignore")


@lru_cache
def get_settings() -> Settings:
    return Settings()
